#pragma once

#if STORM_GC == STORM_GC_SMM

#include "Format.h"
#include <queue>

namespace storm {
	namespace smm {

		class FinalizerPool;

		/**
		 * Class containing the context needed while executing finalizers. Namely:
		 *
		 * - The UThreads we run on other threads to properly run finalizers.
		 * - The cleanup required when all finalizers have finished executing.
		 *
		 * The destructor of this class will make sure to wait for any UThreads running finalizers
		 * and then run any remaining cleanup.
		 */
		class FinalizerContext {
		public:
			FinalizerContext();
			~FinalizerContext();

			// Make sure to run the finalizer for 'obj' on the specified thread.
			void finalize(fmt::Obj *object, const os::Thread &thread);

			// Called by FinalizerPool to register cleanup.
			typedef void (FinalizerPool::*FinalizerPoolFn)(void *aux);
			void cleanup(FinalizerPool *pool, FinalizerPoolFn fn, void *aux);

		private:
			// No copy.
			FinalizerContext(FinalizerContext &);
			FinalizerContext &operator =(FinalizerContext &);

			// Work that should be done when all workers are done.
			class Finish {
			public:
				Finish();

				// Add/remove reference.
				void addRef();
				void release();

				// References.
				size_t refs;

				// Cleanup needed by the finalizer pool, if any.
				FinalizerPool *pool;
				FinalizerPoolFn poolFn;
				void *poolAux;
			};

			// Used if no threads were spawned.
			Finish local;

			// Used if any threads were spawned.
			Finish *shared;

			// Struct for worker threads.
			class Worker {
			public:
				util::Lock lock;

				// Notified for every item in the queue. If 'avail' is raised beyond the elements
				// pushed to 'finalize', that indicates we're done.
				Semaphore avail;

				// What to do when everything is done.
				Finish *finish;

				// Queue of items to finalize.
				std::queue<fmt::Obj *> finalize;

				// Create.
				Worker(Finish *finish);

				// Worker function.
				void CODECALL work();
			};

			// All threads.
			typedef map<uintptr_t, Worker *> Workers;
			Workers workers;
		};

	}
}

#endif
