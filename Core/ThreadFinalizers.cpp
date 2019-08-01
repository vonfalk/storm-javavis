#include "stdafx.h"
#include "ThreadFinalizers.h"
#include "OS/FnCall.h"

namespace storm {

	// This is mostly to be able to properly destroy the lock. It is most likely fine if we don't
	// destroy it anyway.
	void CODECALL destroyThreadFinalizers(ThreadFinalizers *o) {
		o->~ThreadFinalizers();
	}

	const GcType ThreadFinalizers::gcType = {
		GcType::tFixed,
		null, // Type
		address(&destroyThreadFinalizers), // finalizer
		sizeof(ThreadFinalizers), // stride
		1, // number of pointers
		{ OFFSET_OF(ThreadFinalizers, work) } // pointer offsets
	};

	ThreadFinalizers *ThreadFinalizers::create(Engine &e) {
		void *mem = runtime::allocRaw(e, &gcType);
		return new (Place(mem)) ThreadFinalizers(e);
	}

	ThreadFinalizers::ThreadFinalizers(Engine &e)
		: e(e), cleanupRunning(0), work(null) {}

	ThreadFinalizers::~ThreadFinalizers() {}

	void ThreadFinalizers::finalize(const os::Thread &thread, void *object, DtorFn dtor) {
		if (object == null && dtor == null)
			return;

		push(Element(object, dtor));

		if (atomicCAS(cleanupRunning, 0, 1) == 0) {
			// Start the cleanup thread, it is not running.
			ThreadFinalizers *me = this;
			os::FnCall<void, 1> params = os::fnCall().add(me);
			os::UThread::spawn(address(&ThreadFinalizers::cleanup), true, params, &thread);
		}
	}

	void ThreadFinalizers::cleanup() {
		// Run finalizers until we drop!
		while (Element e = pop()) {
			(*e.dtor)(e.object);
		}

		// Signal shutdown.
		atomicWrite(cleanupRunning, 0);
	}


	void ThreadFinalizers::push(const Element &elem) {
		util::Lock::L z(lock);

		if (!work || work->filled >= work->count - 1) {
			GcArray<void *> *a = allocChunk();
			a->v[a->count - 1] = work;
			work = a;
		}

		// Just fill in the current element!
		work->v[work->filled++] = elem.object;
		work->v[work->filled++] = (void *)elem.dtor;
	}

	ThreadFinalizers::Element ThreadFinalizers::pop() {
		util::Lock::L z(lock);

		if (!work)
			return Element();

		size_t id = work->filled;
		Element r(work->v[id - 2], (DtorFn)work->v[id - 1]);

		if (work->filled > 2) {
			work->filled -= 2;
		} else {
			// Remove it, so that we don't have to check next time!
			work = (GcArray<void *> *)work->v[work->count - 1];
		}

		return r;
	}

	GcArray<void *> *ThreadFinalizers::allocChunk() {
		return runtime::allocArray<void *>(e, &pointerArrayType, chunkSize * 2 + 1);
	}
}
