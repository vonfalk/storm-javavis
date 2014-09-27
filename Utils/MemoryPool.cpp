#include "StdAfx.h"
#include "MemoryPool.h"

namespace util {

	MemoryPool::MemoryPool(nat size) : data(new byte[size]), dataSize(size), firstFree(0), allocCount(0) {}

	MemoryPool::~MemoryPool() {
		assert(allocCount == 0);
		delete []data;
	}

	void *MemoryPool::alloc(nat size) {
		if (dataSize - firstFree < size - 1) {
			//Not enough free space. We're filled.
			firstFree = dataSize;
			return null;
		}

		nat allocAt = firstFree + sizeof(void *);
		void **ownerPtr = (void **)&data[firstFree];
		*ownerPtr = this;

		firstFree += sizeof(void *) + size;
		allocCount++;

		return (void *)&data[allocAt];
	}

	void MemoryPool::free(void *memory) {
		assert(allocCount > 0);
		if (--allocCount == 0) {
			firstFree = 0;
		}
	}

	MemoryPool *MemoryPool::getPoolFromAlloc(void *mem) {
		MemoryPool **ptr = (MemoryPool **)mem;
		return ptr[-1];
	}
}
