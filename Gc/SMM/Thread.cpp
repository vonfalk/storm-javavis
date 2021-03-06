#include "stdafx.h"
#include "Thread.h"

#if STORM_GC == STORM_GC_SMM

#include "Arena.h"
#include "Generation.h"

namespace storm {
	namespace smm {

		/**
		 * On Windows, we can use GetThreadContext to retrieve a thread's context when we have suspended it.
		 */


		Thread::Thread(Arena &owner)
			: alloc(owner.nurseryGen()),
			  stacks(os::Thread::current().stacks()) {}

	}
}

#endif
