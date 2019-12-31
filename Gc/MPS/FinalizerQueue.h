#pragma once

#if STORM_GC == STORM_GC_MPS

#include "Core/GcArray.h"

namespace storm {

	class Gc;

	/**
	 * A queue of finalizers waiting to be executed by a particular thread.
	 *
	 * Basically, a list of pointers stored inside a GC-allocated array. The list is split into
	 * chunks, linked together through the first pointer in each list.
	 */
	class FinalizerQueue {
	public:
		FinalizerQueue();

		// Function used for allocating memory.
		typedef void *(AllocFn)(void *data, const GcType *type, size_t count);

		// Head of the stack. Public so that they can be scanned.
		GcArray<void *> *head;

		// Push an element onto the list.
		void push(AllocFn *alloc, void *data, void *obj);

		// Pop an element from the list. Return 'null' if empty.
		void *pop();

	private:
		// Lock for the data structure inside.
		util::Lock lock;

		// Allocate a new chunk.
		GcArray<void *> *allocChunk(AllocFn *alloc, void *data);
	};

}

#endif
