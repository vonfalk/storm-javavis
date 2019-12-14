#include "stdafx.h"
#include "ThreadWin.h"

#if STORM_GC == STORM_GC_SMM && defined(WINDOWS)

namespace storm {
	namespace smm {

		OSThread::OSThread() {
			threadId = currentId();
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

		void OSThread::requestStop() {
			if (stopCount++ == 0) {
				if (SuspendThread(handle) == (DWORD)-1) {
					assert(false, L"SuspendThread failed!");
				}
			}
			stopQueries++;
		}

		void OSThread::ensureStop() {
			assert(stopQueries > 0, L"ensureStop called without requestStop");
			--stopQueries;

			// According to https://devblogs.microsoft.com/oldnewthing/20150205-00/?p=44743, this
			// makes sure that the thread is actually stopped.
			context.ContextFlags = CONTEXT_INTEGER | CONTEXT_CONTROL;
			if (GetThreadContext(handle, &context) == FALSE) {
				assert(false, L"GetThreadContext failed!");
			}
		}

		void OSThread::start() {
			if (stopCount > 0) {
				if (--stopCount == 0) {
					if (ResumeThread(handle) == (DWORD)-1) {
						assert(false, L"ResumeThread failed!");
					}
				}
			}
		}

	}
}

#endif
