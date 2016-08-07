#include "stdafx.h"
#include "Thread.h"
#include "Engine.h"
#include "Core/GcType.h"

namespace storm {

	/**
	 * Implement some functions only needed in the compiler.
	 */


	static void destroyThread(Thread *t) {
		t->~Thread();
	}

	static const GcType firstDesc = {
		GcType::tFixed,
		null, // Type
		address(&destroyThread), // Finalizer
		sizeof(Thread), // stride/size
		0, // # of offsets
		{},
	};

	void *Thread::operator new(size_t s, First d) {
		return d.e.gc.alloc(&firstDesc);
	}

	void Thread::operator delete(void *mem, First d) {}


	STORM_DEFINE_THREAD(Compiler);

}
