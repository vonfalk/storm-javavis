#include "stdafx.h"
#include "ThreadPosix.h"

#if STORM_GC == STORM_GC_SMM && defined(POSIX)

#include <signal.h>
#include <ucontext.h>

namespace storm {
	namespace smm {

		// Signals to use.
		static int sigStop = SIGXCPU;

		// Number of threads currently active. If >= 0, we have inserted a global signal handler.
		static size_t activeThreadCount = 0;

		// Old signal handler.
		static siginfo_t oldStopSig;

		// Per-thread information.
		struct ThreadInfo {
			// Number of references.
			size_t refs;

			// Pointer to the thread-local variable, so that we can clear it.
			ThreadInfo **self;

			// If stopped, pointer to the current context.
			ucontext_t *context;

			// Semaphore we wait on when we're stopped.
			Semaphore onResume;

			// Semaphore notified when the thread is actually stopped.
			Semaphore onStop;

			// Create.
			ThreadInfo() : refs(0) {}
		};
		static THREAD ThreadInfo *tlInfo = null;

		// Handle signals.
		static void onSigStop(int signal, siginfo_t *info, void *context) {
			(void)signal;
			(void)info;

			if (tlInfo) {
				tlInfo->context = (ucontext_t *)context;
				tlInfo->onStop.up();
				tlInfo->onResume.down();
			}
		}


		OSThread::OSThread() : tid(pthread_self()), stopCount(0) {
			if (atomicIncrement(activeThreadCount) == 1) {
				// We're first.
				struct sigaction stopSig;
				stopSig.sa_sigaction = &onSigStop;
				stopSig.sa_flags = SA_SIGINFO | SA_RESTART;
				sigemptyset(&stopSig.sa_mask);
				sigaction(sigStop, &stopSig, &oldStopSig);
			}

			if (!tlInfo) {
				tlInfo = new ThreadInfo();
				tlInfo->self = &tlInfo;
			}

			info = tlInfo;
			atomicIncrement(info->refs);
		}

		OSThread::~OSThread() {
			if (atomicDecrement(info->refs) == 0) {
				*info->self = null;
				delete info;
			}

			if (atomicDecrement(activeThreadCount) == 0) {
				// We're last.
				sigaction(sigStop, &oldStopSig);
			}
		}

		void OSThread::requestStop() {
			assert(false, L"TODO");
		}

		void OSThread::ensureStop() {
			assert(false, L"TODO");
		}

		void OSThread::start() {
			// assert(false, L"TODO");
		}

	}
}

#endif
