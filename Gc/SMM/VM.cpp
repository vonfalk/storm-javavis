#include "stdafx.h"
#include "VM.h"

#if STORM_GC == STORM_GC_SMM

#include "VMWin.h"
#include "VMPosix.h"

namespace storm {
	namespace smm {

#if defined(WINDOWS)

		VM *VM::create() {
			return VMWin::create();
		}

#elif defined (POSIX)

		VM *VM::create() {
			return VMPosix::create();
		}

#else
#error "Please implement the virtual memory functions for your platform!"
#endif

	}
}

#endif
