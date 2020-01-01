#pragma once

#include "Zero.h"
#include "MPS/Impl.h"
#include "SMM/Impl.h"

namespace storm {

	/**
	 * Additional functionality useful for implementations. Should reside in Gc.h if it wasn't
	 * included from other projects.
	 */

	// Get the thread data for a thread, given a pointer to the implementation. Returns
	// "default" if the thread is not registered with the GC.
	GcImpl::ThreadData threadData(GcImpl *from, const os::Thread &thread, const GcImpl::ThreadData &def);

}
