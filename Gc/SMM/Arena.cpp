#include "stdafx.h"
#include "Arena.h"

#if STORM_GC == STORM_GC_SMM

namespace storm {
	namespace smm {

		Arena::Arena(size_t initialSize) {
			platformInit();

			// TODO: Reserve virtual memory according to the initial size; we want to be able to
			// tell other parts of the system whether or not a pointer is inside memory we control
			// or not. This is faster if we control a few large blocks of memory, rather than many
			// smaller blocks.
		}

		Arena::~Arena() {}



#if defined(WINDOWS)

		void Arena::platformInit() {
			SYSTEM_INFO info;
			GetSystemInfo(&info);

			allocGranularity = info.dwAllocationGranularity;
			pageSize = info.dwPageSize;
		}

#elif defined(POSIX)

#error "Please implement Arena for Posix systems!"

#else
#error "Please implement the Arena for your platform!"
#endif

	}
}

#endif
