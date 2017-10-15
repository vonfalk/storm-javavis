#pragma once
#include "DwarfEh.h"
#include "Utils/Lock.h"

namespace code {
	namespace x64 {
		STORM_PKG(core.asm.x64);

		class DwarfChunk;

		/**
		 * Storage for FDE headers for exception data in a way that does not degrade performance of
		 * the GC while providing fast lookups of exception data.
		 *
		 * Currently implemented as a set of chunks. Each chunk contains a sorted sequence of FDE
		 * records, so that it is possble to perform a binary search within each chunk. The
		 * different chunks bear no relation to each other, so all have to be searched for every
		 * request. Therefore, the size of the chunks must be tuned properly to gain a reasonable
		 * performance for FDE lookup while not wasting too much memory.
		 *
		 * While entries are being sorted, the actual FDE is at a static location within each
		 * chunk. This is so that we can easily and quickly update pointers duing scanning in the
		 * GC. Updating pointers in this manner will invalidate the sorted sequence within each
		 * chunk. This is indicated by the 'sorted' variable inside each chunk.
		 *
		 * It is also important to consider the kind of concurrency present in this kind of
		 * structure. The entire structure is protected by locks for 'regular' accesses. This solves
		 * most, but not all of the problems. The GC may still modify pointers at any time, and
		 * since we're performing binary searches on these pointers, the binary searches may
		 * erroneously fail in some cases. This is solved by examining the 'sorted' value whenever a
		 * binary search failed, and retrying the search in such case. This is similar to what is
		 * done inside the hash maps in the Storm standard library.
		 */
		class DwarfTable {
		public:
			// Create.
			DwarfTable();

			// Destroy.
			~DwarfTable();

			// Allocate a new FDE for the function at address 'fn', which is expected to be a
			// function allocated on the GC heap.
			FDE *alloc(const void *fn);

			// Free a FDE.
			void free(FDE *fde);

			// Find the FDE for a specific location (might be inside of a function). Returns 'null'
			// on failure.
			FDE *find(const void *pc);

		private:
			// Lock for this class.
			util::Lock lock;

			// All allocated chunks. The most recently allocated chunk is last.
			vector<DwarfChunk *> chunks;
		};

		// The global table of DWARF information.
		extern DwarfTable dwarfTable;

		/**
		 * A single chunk within the DwarfTable.
		 *
		 * Implements a simple memory allocator. Since all FDEs are the same size, we can easily do
		 * this using a static array with a free list stored inside the unused allocations.
		 */
		class DwarfChunk {
		public:
			// Create.
			DwarfChunk();

			// Allocate a new FDE. Returns 'null' if no space is available.
			FDE *alloc(const void *fn);

			// Free a FDE stored in here.
			void free(FDE *fde);

			// Find a FDE.
			FDE *find(const void *pc);

			// Find the owner of a FDE allocated in a DwarfTable.
			static DwarfChunk *owner(FDE *fde);

			// Update the pointer to the function inside of a FDE.
			static void updateFn(FDE *fde, const void *fn);

		private:
			// A single entry stored here. A pointer to the chunk is also stored so that 'sorted'
			// can be updated easily from the GC simply by knowing the pointer to a FDE.
			struct Entry {
				// The data.
				FDE data;

				// Either a pointer to the DwarfChunk or a pointer to the next free entry in the
				// free list.
				union {
					Entry *nextFree;
					DwarfChunk *owner;
				} ptr;

				// Is 'ptr' covered by this entry?
				Bool contains(const void *pc);
			};

			// Comparision object.
			struct Compare;

			// The CIE header used inside all FDEs inside this chunk.
			CIE header;

			// The blocks that we allocate.
			Entry data[CHUNK_COUNT];

			// First entry in the free list.
			Entry *firstFree;

			// Array of pointers to the entries. Sorted according to addresses inside 'data' if
			// 'updated' is true. Contains 'null' if the chunk is not completely filled.
			Entry *sorted[CHUNK_COUNT];

			// Is this chunk properly sorted? (0 or 1)
			Nat updated;

			// Perform a binary search in attempt to find the desired function.
			FDE *search(const void *pc);

			// Update the sorted array required for the binary search. Also searches for 'fn' during
			// the sorting process.
			FDE *update(const void *pc);
		};

	}
}
