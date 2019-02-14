#include "stdafx.h"
#include "VM.h"

#if STORM_GC == STORM_GC_SMM

#include "VMWin.h"

namespace storm {
	namespace smm {

#if defined(WINDOWS)

		VM *VM::create() {
			return VMWin::create();
		}

#elif defined (POSIX)

#error "Please implement the virtual memory functions for POSIX!"

#else
#error "Please implement the virtual memory functions for your platform!"
#endif

	}
}

#endif
