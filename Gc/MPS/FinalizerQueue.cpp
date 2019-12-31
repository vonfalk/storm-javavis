#include "stdafx.h"
#include "FinalizerQueue.h"

#if STORM_GC == STORM_GC_MPS

#include "Gc/Gc.h"

namespace storm {

	// Chunk size.
	static const size_t chunkSize = 100;

	FinalizerQueue::FinalizerQueue() : head(null) {}

	void FinalizerQueue::push(AllocFn *alloc, void *data, void *obj) {
		util::Lock::L z(lock);

		if (!head) {
			// Empty.
			head = allocChunk(alloc, data);
		}

		if (head->filled == head->count) {
			// Last chunk full?
			GcArray<void *> *c = allocChunk(alloc, data);
			head->v[0] = c;
			head = c;
		}

		head->v[head->filled++] = obj;
	}

	void *FinalizerQueue::pop() {
		util::Lock::L z(lock);

		if (!head) {
			// Empty.
			return null;
		}

		if (head->filled == 1) {
			// Empty (should not happen).
			head = null;
			return null;
		}

		void *r = head->v[--head->filled];

		if (head->count == 1) {
			// We took the last one.
			head = null;
		}

		return r;
	}

	GcArray<void *> *FinalizerQueue::allocChunk(AllocFn *alloc, void *data) {
		GcArray<void *> *r = (GcArray<void *> *)(*alloc)(data, &pointerArrayType, chunkSize);

		// For the 'next' pointer.
		r->filled = 1;

		return r;
	}

}

#endif
