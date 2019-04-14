#pragma once

#if STORM_GC == STORM_GC_SMM

#include "VM.h"
#include "Utils/Bitwise.h"
#include "AddrSet.h"
#include <vector>

namespace storm {
	namespace smm {

		/**
		 * Description of a piece of allocated memory.
		 */
		class Chunk {
		public:
			Chunk() : at(null), size(0) {}
			Chunk(void *at, size_t size) : at(at), size(size) {}

			// Empty?
			bool empty() const { return at == null; }
			bool any() const { return at != null; }

			// Start of the memory.
			void *at;

			// Size, in bytes.
			size_t size;
		};

		wostream &operator <<(wostream &to, const Chunk &c);


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
			// Create, with a backing VM instance. Making an initial reservation of approx. 'init' bytes.
			VMAlloc(VM *backend, size_t initialSize);

			// Destroy.
			~VMAlloc();

			// The page size of this system.
			const size_t pageSize;

			/**
			 * Memory management.
			 */

			// Allocate at least 'size' bytes and associate it with the given identifier.
			Chunk alloc(size_t size, byte id);

			// Free a chunk of memory. Not necessarily the exact same chunk as allocated previously.
			void free(Chunk chunk);

			// Get the identifier for an allocation. Assumes 'ptr' was previously allocated here.
			inline byte identifier(void *addr) const {
				return infoData(info[infoOffset(addr)]);
			}

			// Check if a pointer refers to an object managed by this instance. Returns 'true' if
			// the pointer refers to a block currently allocated here.
			inline bool has(void *addr) {
				size_t a = size_t(addr);
				if (a >= minAddr && a < maxAddr)
					return infoUsed(info[infoOffset(addr)]);
				return false;
			}

			// Create an AddrSet that is large enough to contain addresses in the entire arena
			// managed by this instance.
			template <class AddrSet>
			AddrSet addrSet() const {
				return AddrSet(minAddr, maxAddr);
			}


			/**
			 * API for the Arena class.
			 */

			// Check for writes to any blocks we manage. Some backends only update the 'fUpdated'
			// flag for blocks this function is called, others do it during the actual write.
			void checkWrites(void **arenaBuffer);


			/**
			 * API to the VM backends.
			 */

			// Get all chunks in this allocator. Mainly intended for VM subclasses.
			const vector<Chunk> &chunkList() const { return reserved; }

			// Mark all blocks in the contained addresses as 'updated'.
			void markBlockWrites(void **addr, size_t count);

		private:
			// No copy!
			VMAlloc(const VMAlloc &o);
			VMAlloc &operator =(const VMAlloc &o);

			// VM backend.
			VM *vm;

			// Keep track of all reserved memory. We strive to keep this array small. Otherwise, the
			// "contains pointer" query will be expensive.
			vector<Chunk> reserved;

			// Min- and max address managed in this instance.
			size_t minAddr;
			size_t maxAddr;

			// Update 'minAddr' and 'maxAddr'.
			void updateMinMax(size_t &minAddr, size_t &maxAddr);

			// Add a chunk of reserved memory to our pool.
			void addReserved(Chunk chunk);


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
			 * Bit 1: Contents altered since last check?
			 * Bit 2-7: User data.
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

			// Get the address of a particular offset in 'index'.
			inline void *infoPtr(size_t index) const {
				size_t addr = minAddr + (index << vmAllocBits);
				return (void *)addr;
			}

			// Constants for some useful info values.
			enum {
				INFO_FREE = 0x00,
				INFO_USED = 0x01,
				INFO_WRITTEN = 0x02
			};

			// Check if a particular byte in 'info' is marked as 'used'.
			static inline bool infoUsed(byte b) {
				return b & 0x01;
			}

			// Check if a particular byte in 'info' is marked as 'written'.
			static inline bool infoWritten(byte b) {
				return (b >> 1) & 0x01;
			}

			// Get the data portion of a byte in 'info'.
			static inline byte infoData(byte b) {
				return b >> 2;
			}

		};

	}
}

#endif
