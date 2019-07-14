#include "stdafx.h"
#include "ThreadPosix.h"

#if STORM_GC == STORM_GC_SMM && defined(POSIX)

namespace storm {
	namespace smm {

		OSThread::OSThread() : tid(pthread_self()), stopCount(0) {}

		OSThread::~OSThread() {}

		void OSThread::requestStop() {
			assert(false, L"TODO");
		}

		void OSThread::ensureStop() {
			assert(false, L"TODO");
		}

		void OSThread::start() {
			assert(false, L"TODO");
		}

	}
}

#endif
