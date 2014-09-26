#pragma once

#include "Object.h"
#include "MemoryPool.h"

#include "Lock.h"

namespace util {

	//Pre-allocates memory using new to create a few pools of 
	//memory. The pools themselves are allocated until they are
	//full and are then considered full until all allocations
	//within them are freed.
	//This makes this allocator suitable for data allocated and
	//then placed in a queue, where allocations and destructions
	//are almost in the same order and about the same size.
	//The number of pools is managed automatically to ensure
	//that allocations will not fail before the process' is out
	//of memory, and so that the pools do not take up too much memory.
	//However, the calls to new and delete are minimized.
	class PoolAllocator : NoCopy {
	public:
		//Create the allocator with an expected allocation size (in bytes).
		PoolAllocator(nat allocSize = 30);

		//Destroy the allocator. The allocator will ASSERT if any memory is
		//still allocated.
		~PoolAllocator();

		//Allocate memory
		void *alloc(nat size);

		//Deallocate memory. The memory must have been allocated
		//by this allocator beforehand.
		void free(void *memory);

	private:
		//Tweakable constants:
		static const nat ALLOCS_PER_POOL = 100; //Expect ~100 allocations per pool
		static const nat MAX_FREE_PER_USED = 3; //The maxium ration of free pools vs used pools.

		Lock lock;

		class PoolList {
		public:
			PoolList(nat size) : pool(size), next(null), prev(null) {}
			PoolList *next, *prev;
			MemoryPool pool;
		};

		//The size of pools.
		nat poolSize;

		//A list of non-full pools. The first will be used in the next allocation
		//unless it gets full.
		nat numEmptyPools;
		PoolList *emptyPools;

		//A list of the filled pools.
		nat numFilledPools;
		PoolList *filledPools;

		void checkMaxRatio();
	};

}