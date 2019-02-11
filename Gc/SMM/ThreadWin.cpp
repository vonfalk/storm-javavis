#include "stdafx.h"
#include "ThreadWin.h"

#if STORM_GC == STORM_GC_SMM && defined(WINDOWS)

namespace storm {
	namespace smm {

		OSThread::OSThread() {
			HANDLE proc = GetCurrentProcess();
			DWORD access = THREAD_GET_CONTEXT | THREAD_SUSPEND_RESUME | THREAD_QUERY_INFORMATION;
			if (!DuplicateHandle(proc, GetCurrentThread(), proc, &handle, access, FALSE, 0)) {
				assert(false, L"DuplicateHandle failed!");
			}
			stopCount = 0;
		}

		OSThread::~OSThread() {
			CloseHandle(handle);
		}

		void OSThread::stop() {
			if (stopCount++ == 0) {
				if (SuspendThread(handle) == (DWORD)-1) {
					assert(false, L"SuspendThread failed!");
				}
			}
		}

		void OSThread::start() {
			if (--stopCount == 0) {
				if (ResumeThread(handle) == (DWORD)-1) {
					assert(false, L"ResumeThread failed!");
				}
			}
		}

	}
}

#endif
