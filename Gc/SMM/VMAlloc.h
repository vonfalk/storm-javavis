#pragma once

#if STORM_GC == STORM_GC_SMM

#include "VM.h"
#include "Gc/MemorySummary.h"
#include "Utils/Bitwise.h"
#include "AddrSet.h"
#include "Chunk.h"
#include <vector>

namespace storm {
	namespace smm {

		/**
		 * Management of virtual memory allocations.
		 *
		 * Reserves a fairly large piece of virtual memory for future memory allocations and manages
		 * memory in the reserved space. Memory is allocated in chunks of about 64k (determined by
		 * 'vmAllocMinSize'). The VMAlloc additionally allows associating 6 bits of additional data
		 * with each block in order to allow the rest of the system to quickly identify which
		 * generation owns a particular piece of memory during garbage collection. Additionally, the
		 * system allows tracking writes to individual allocations to reduce the amount of memory
		 * that needs scanning during garbage collection.
		 */
		class VMAlloc {
		public:
			// Create. Making an initial reservation of approx. 'init' bytes.
			VMAlloc(size_t initialSize);

			// Destroy.
			~VMAlloc();

			/**
			 * Memory management.
			 */

			// Allocate at least 'size' bytes and associate it with the given identifier.
			Chunk alloc(size_t size, byte id);

			// Allocate at least 'min' bytes, but preferably 'preferred' bytes and associate it with
			// the given identifier.
			Chunk alloc(size_t min, size_t preferred, byte id);

			// Free a chunk of memory. Not necessarily the exact same chunk as allocated previously.
			void free(Chunk chunk);

			// Get the identifier for an allocation. Assumes 'ptr' was previously allocated here.
			inline byte identifier(void *addr) const {
				return infoData(info[infoOffset(addr)]);
			}

			// Get the identifier for an allocation safely. Returns 0xFF on failure.
			inline byte safeIdentifier(void *addr) const {
				size_t a = size_t(addr);
				// We're doing this without branches for performance. It is called quite frequently
				// during scanning, and the branch predictor will not do a good job there.
				// This branch-free implementation seems slower than the original (not by much).
				// bool mask = (a >= minAddr) & (a < maxAddr);
				// byte data = info[infoOffset(addr) * mask] * mask;

				// byte used = (~data) & 0x01;
				// return infoData(data) | (used * 0xFF);

				// Original implementation:
				if (a >= minAddr && a < maxAddr) {
					byte data = info[infoOffset(addr)];
					if (infoClientUse(data))
						return infoData(data);
				}
				return 0xFF;
			}

			// Check if a pointer refers to an object managed by this instance. Returns 'true' if
			// the pointer refers to a block currently allocated here.
			inline bool has(void *addr) const {
				size_t a = size_t(addr);
				if (a >= minAddr && a < maxAddr)
					return infoClientUse(info[infoOffset(addr)]);
				return false;
			}

			// Create an AddrSet that is large enough to contain addresses in the entire arena
			// managed by this instance.
			template <class AddrSet>
			AddrSet addrSet() const {
				return AddrSet(minAddr, maxAddr);
			}


			/**
			 * Memory protection.
			 */

			// Protect a chunk of memory in order to detect writes to it. Will round the boundaries
			// of the chunk to a suitable granularity.
			void watchWrites(Chunk chunk);

			// Stop watching writes for a chunk of memory. Immediately marks all parts of the memory
			// as 'potentially altered'. This is useful to do when one page in a range has been
			// changed, and we know we will change lots of adjacent pages without hitting the
			// pagefault barrier too often.
			void stopWatchWrites(Chunk chunk);

			// Check for writes to a range of addresses. Returns true if any addresses in the range
			// are either unprotected, or if they were protected but have been written to.
			bool anyWrites(Chunk chunk);


			/**
			 * API to the VM backends.
			 */

			// Get all chunks in this allocator. Mainly intended for VM subclasses.
			const vector<Chunk> &chunkList() const { return reserved; }

			// Mark 'ptr' as written. We will also reset protection on these pages to
			// read/write. Returns 'true' if this was a previously protected range of memory.
			bool onProtectedWrite(void *ptr);


			/**
			 * Misc.
			 */

			// Fill the memory summary information we can provide.
			void fillSummary(MemorySummary &summary) const;

			// Dump the allocation information.
			void dbg_dump();

		private:
			// No copy!
			VMAlloc(const VMAlloc &o);
			VMAlloc &operator =(const VMAlloc &o);

			// VM backend.
			VM *vm;

			// Keep track of all reserved memory. We strive to keep this array small. Otherwise, the
			// "contains pointer" query will be expensive. Elements are sorted by their starting
			// address, and not overlapping with each other.
			vector<Chunk> reserved;

			// Min- and max address managed in this instance.
			size_t minAddr;
			size_t maxAddr;

			// Update 'minAddr' and 'maxAddr'.
			void updateMinMax(size_t &minAddr, size_t &maxAddr);

			// Add a chunk of reserved memory to our pool.
			void addReserved(Chunk chunk);

			// Create a new info data structure for the specified chunk.
			void createInfo(Chunk chunk);

			// Resize (and possibly move) the info data structure to acommodate a newly allocated chunk.
			void resizeInfo(Chunk chunk);

			// Mark a range of addresses as inaccessible.
			void markInaccessible(void *from, void *to);

			// Compute the total number of pieces managed by this instance.
			size_t totalPieces() const;

			// Attempt to allocate more memory, a minimum of 'minPieces' usable pieces, to
			// serve a pending allocation request. Returns 'true' if successful.
			bool expandAlloc(size_t minPieces);

			// Attempt to allocate 'pieces' pieces of virtual memory from the OS. We prefer to
			// allocate it between any chunks we've already allocated if possible. Otherwise, we
			// prefer before or after the current allocation, or as a last resort the new allocation
			// may be anywhere.
			Chunk attemptAlloc(size_t pieces);


			/**
			 * Contents of the 'info' member describing each chunk.
			 *
			 * Each byte represents 'blockMinSize' bytes of memory, starting from 'minAddr'. Each
			 * byte indicates whether or not that particular block is used, and other data that can
			 * be retrieved from the public API (e.g. which generation the memory belongs to and
			 * other information that is used to quickly determine if a particular pointer should be
			 * collected or not).
			 *
			 * The content of each byte is as follows:
			 * Bit 0: In use (1) or free (0).
			 * Bit 1: Contents maybe altered since the last check? Only clear for write-protected memory.
			 * Bit 2-7: User data.
			 *
			 * Note: If only bit 1 is set, the block is marked as in use by the memory management
			 * system, as a written, unallocated block does not make sense.
			 */

			// Current location of the memory information data. Allocated in one of the
			// chunks. Covers 'minAddr' to 'maxAddr' regardless of the number of chunks actually in
			// use.
			byte *info;

			// Last index in 'info' we allocated anything at.
			size_t lastAlloc;

			// Get the size of the info block, given a range of addresses. Note: We round down so
			// that any partial last block will not be contained in the info, since it will be
			// unusable anyway.
			inline size_t infoCount(size_t min, size_t max) const {
				return (max - min) >> vmAllocBits;
			}
			inline size_t infoCount() const {
				return infoCount(minAddr, maxAddr);
			}

			// Get the index in 'info' for a particular address. Undefined for ranges outside [minAddr, maxAddr].
			inline size_t infoOffset(void *mem) const {
				size_t ptr = size_t(mem);
				return (ptr - minAddr) >> vmAllocBits;
			}
			inline size_t infoOffset(void *mem, size_t minAddr) const {
				size_t ptr = size_t(mem);
				return (ptr - minAddr) >> vmAllocBits;
			}

			// Get the address of a particular offset in 'index'.
			inline void *infoPtr(size_t index) const {
				size_t addr = minAddr + (index << vmAllocBits);
				return (void *)addr;
			}
			inline void *infoPtr(size_t index, size_t minAddr) const {
				size_t addr = minAddr + (index << vmAllocBits);
				return (void *)addr;
			}

			// Constants for some useful info values.
			enum {
				INFO_FREE = 0x00,
				INFO_USED_CLIENT = 0x01,
				INFO_USED_WRITTEN = 0x03,
				INFO_USED_INTERNAL = 0x02
			};

			// Check if a particular byte in 'info' is marked as 'used', either by us or by the client.
			static inline bool infoInUse(byte b) {
				return (b & 0x03) != 0;
			}

			// Check if a particular byte in 'info' is marked as 'used' by the client.
			static inline bool infoClientUse(byte b) {
				return (b & 0x01) == 1;
			}

			// Check if a particular byte in 'info' is marked as 'written'.
			static inline bool infoWritten(byte b) {
				return (b & 0x03) == INFO_USED_WRITTEN;
			}

			// Check if a particular byte in 'info' is unchanged and protected.
			static inline bool infoProtected(byte b) {
				return (b & 0x03) == INFO_USED_CLIENT;
			}

			// Get the data portion of a byte in 'info'.
			static inline byte infoData(byte b) {
				return b >> 2;
			}

		public:
			// The page size of this system (must be initialized after 'vm', so it is moved here).
			const size_t pageSize;

		};

	}
}

#endif
