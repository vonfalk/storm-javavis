#include "stdafx.h"
#include "PoolAllocator.h"
#include "Memory.h"

namespace util {

	PoolAllocator::PoolAllocator(nat allocSize /* = 30 */) : poolSize(allocSize * ALLOCS_PER_POOL), emptyPools(null), filledPools(null), numEmptyPools(0), numFilledPools(0) {}

	PoolAllocator::~PoolAllocator() {
		while (emptyPools != null) {
			PoolList *next = emptyPools->next;
			delete emptyPools;
			emptyPools = next;
		}

		//This should not be the case, as these will certainly assert.
		while (filledPools != null) {
			PoolList *next = filledPools->next;
			delete filledPools;
			filledPools = next;
		}
	}

	void *PoolAllocator::alloc(nat size) {
		Lock::L l(lock);
		void *allocated = null;
		while (allocated == null) {
			if (emptyPools == null) {
				//No free pools, we need a new one!
				emptyPools = new PoolList(poolSize);
				numEmptyPools++;
			}

			allocated = emptyPools->pool.alloc(size);

			if (emptyPools->pool.filled()) {
				PoolList *moving = emptyPools;
				emptyPools = emptyPools->next;
				moving->prev = null;
				numEmptyPools--;

				moving->next = filledPools;
				moving->prev = null;
				if (filledPools) filledPools->prev = moving;
				filledPools = moving;
				numFilledPools++;
			}
		}
		return allocated;
	}

	void PoolAllocator::free(void *memory) {
		Lock::L l(lock);

		MemoryPool *allocatedFrom = MemoryPool::getPoolFromAlloc(memory);
		bool wasFilled = allocatedFrom->filled();

		allocatedFrom->free(memory);

		if (wasFilled && !allocatedFrom->filled()) {
			PoolList *allocFrom = BASE_PTR(PoolList, allocatedFrom, pool);

			if (allocFrom->prev) allocFrom->prev->next = allocFrom->next;
			if (allocFrom->next) allocFrom->next->prev = allocFrom->prev;
			if (allocFrom == filledPools) filledPools = filledPools->next;
			numFilledPools--;

			allocFrom->next = emptyPools;
			allocFrom->prev = null;
			if (emptyPools) emptyPools->prev = allocFrom;
			emptyPools = allocFrom;
			numEmptyPools++;

			checkMaxRatio();
		}
	}

	void PoolAllocator::checkMaxRatio() {
		PoolList *current = emptyPools;
		while (numEmptyPools > max(nat(1), numFilledPools) * MAX_FREE_PER_USED && current != null) {
			if (current->pool.empty()) {
				PoolList *toRemove = current;
				current = current->next;

				if (toRemove->next) toRemove->next->prev = toRemove->prev;
				if (toRemove->prev) toRemove->prev->next = toRemove->next;
				if (toRemove == emptyPools) emptyPools = emptyPools->next;
				delete toRemove;

				numEmptyPools--;

			} else {
				current = current->next;
			}
		}
	}
}
