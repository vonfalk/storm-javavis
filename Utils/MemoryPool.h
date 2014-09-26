#pragma once

#include "Object.h"

namespace util {

	//A specific memory pool used by the PoolAllocator. The pool
	//is allocated in a linear fashion until it is full. Further allocations
	//will then fail until the pool is entirely empty.
	class MemoryPool : NoCopy {
	public:
		//Create the pool with a specific size. This malloc's the memory.
		MemoryPool(nat size);

		//Destroy the pool. This will ASSERT if the pool is not empty.
		~MemoryPool();

		//Allocate memory here. Returns null if the pool is full.
		void *alloc(nat size);

		//Returns true if this pool is full. It is considered full after the first failing alloc
		inline bool filled() const { return firstFree == dataSize; }

		//Returns true if no memory is allocated in this pool.
		inline bool empty() const { return firstFree == 0; }

		//Get the pool which allocated the memory.
		static MemoryPool *getPoolFromAlloc(void *mem);

		//Free memory. Sets "filled" to false if the pool is entirely freed.
		void free(void *memory);
	private:
		//The allocated data and its size.
		byte *data;
		nat dataSize;

		//The first free memory address, relative "data".
		nat firstFree;

		//The number of allocations in this pool.
		nat allocCount;
	};

}