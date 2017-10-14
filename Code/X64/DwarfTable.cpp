#include "stdafx.h"
#include "DwarfTable.h"
#include "Utils/Memory.h"

namespace code {
	namespace x64 {

		/**
		 * The entire table.
		 */

		DwarfTable dwarfTable;

		DwarfTable::DwarfTable() {}

		DwarfTable::~DwarfTable() {
			for (size_t i = 0; i < chunks.size(); i++)
				delete chunks[i];
		}

		FDE *DwarfTable::alloc(const void *fn) {
			util::Lock::L z(lock);

			for (size_t i = chunks.size(); i > 0; i--) {
				if (FDE *n = chunks[i-1]->alloc(fn))
					return n;
			}

			// No room anywhere. Allocate a new chunk!
			DwarfChunk *n = new DwarfChunk();
			chunks.push_back(n);
			return n->alloc(fn);
		}

		void DwarfTable::free(const void *fn) {
			util::Lock::L z(lock);

			FDE *f = find(fn);
			DwarfChunk::owner(f)->free(f);
		}

		FDE *DwarfTable::find(const void *pc) {
			util::Lock::L z(lock);

			for (Nat i = 0; i < chunks.size(); i++)
				if (FDE *f = chunks[i]->find(pc))
					return f;
			return null;
		}

		/**
		 * A single chunk.
		 */

		DwarfChunk::DwarfChunk() {
			header.init();
			header.setLen(&data[0]);

			// Build the list of free FDEs.
			firstFree = &data[0];
			for (Nat i = 0; i < CHUNK_COUNT - 1; i++) {
				data[i].ptr.nextFree = &data[i+1];
			}
			data[CHUNK_COUNT-1].ptr.nextFree = null;

			// Clear 'sorted'.
			for (Nat i = 0; i < CHUNK_COUNT; i++) {
				sorted[i] = null;
			}

			// Remember that we're good to go!
			atomicWrite(updated, 1);
		}

		FDE *DwarfChunk::alloc(const void *fn) {
			if (firstFree == null)
				return null;

			Entry *use = firstFree;
			firstFree = use->ptr.nextFree;

			// Initialize the newly found data.
			use->ptr.owner = this;
			use->data.codeStart() = fn;
			use->data.codeSize() = runtime::codeSize(fn);
			use->data.augSize() = 0;
			use->data.setCie(&header);
			use->data.setLen(&use->ptr.nextFree);

			// Remember that we need to update 'sorted' and return.
			atomicWrite(updated, 0);
			return &use->data;
		}

		void DwarfChunk::free(FDE *fde) {
			assert(fde >= &data[0].data && fde < &data[CHUNK_COUNT].data);

			Entry *e = BASE_PTR(Entry, fde, data);
			assert(e->ptr.owner == this);
			e->ptr.nextFree = firstFree;
			firstFree = e;

			// Now, we need to re-sort the array again.
			atomicWrite(updated, 0);
		}

		FDE *DwarfChunk::find(const void *pc) {
			if (atomicRead(updated) == 0) {
				// Sort the data and find the value using a linear search.
				return update(pc);
			}

			FDE *result = search(pc);
			if (result)
				return result;

			if (atomicRead(updated) == 0) {
				// The pointers were updated during the search. Fall back to the slow implementation.
				result = update(pc);
			}

			return result;
		}

		DwarfChunk *DwarfChunk::owner(FDE *fde) {
			Entry *e = BASE_PTR(Entry, fde, data);
			return e->ptr.owner;
		}

		void DwarfChunk::updateFn(FDE *fde, const void *fn) {
			DwarfChunk *chunk = owner(fde);
			if (fde->codeStart() != fn) {
				atomicWrite(chunk->updated, 0);
				fde->codeStart() = fn;
				atomicWrite(chunk->updated, 0);
			}
		}

		Bool DwarfChunk::Entry::contains(const void *pc) {
			size_t addr = size_t(data.codeStart());
			size_t p = size_t(pc);

			return addr <= p && p < addr + data.codeSize();
		}

		struct DwarfChunk::Compare {
			bool operator ()(Entry *a, Entry *b) const {
				size_t as = size_t(a->data.codeStart());
				size_t bs = size_t(b->data.codeStart());
				return as < bs;
			}
			bool operator ()(Entry *a, const void *pc) const {
				// All 'null' values are in the higher indices, so treat 'a = null' as being a large value.
				if (a == null)
					return false;
				size_t as = size_t(a->data.codeStart());
				size_t bs = size_t(pc);
				return as < bs;
			}
		};

		FDE *DwarfChunk::search(const void *pc) {
			Entry **iter = std::lower_bound(sorted, sorted + CHUNK_COUNT, pc, Compare());
			// We need to examine both 'iter' and 'iter - 1'.
			if (iter != sorted + CHUNK_COUNT) {
				Entry *found = *iter;
				if (found && found->contains(pc))
					return &found->data;
			}

			if (iter != sorted) {
				Entry *found = *(iter - 1);
				if (found && found->contains(pc))
					return &found->data;
			}

			return null;
		}

		FDE *DwarfChunk::update(const void *pc) {
			FDE *result = null;

			// Put all entries into 'sorted'.
			Nat used = 0;
			for (Nat i = 0; i < CHUNK_COUNT; i++) {
				// Is it in use?
				if (data[i].ptr.owner == this) {
					sorted[used++] = &data[i];

					// Is it the one we're looking for?
					if (data[i].contains(pc))
						result = &data[i].data;
				}
			}

			// Set the rest of the elements to 'null'.
			for (Nat i = used; i < CHUNK_COUNT; i++)
				sorted[i] = null;

			// Reset the 'updated' flag now, so that any changes the GC makes during the sorting
			// will count as if they were done after the sorting. Otherwise, things may break.
			atomicWrite(updated, 1);

			// Sort!
			std::sort(sorted, sorted + used, Compare());

			// Done! Hopefully the array is not altered until next time some information is needed!
			return result;
		}

	}
}
