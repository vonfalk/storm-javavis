#include "stdafx.h"
#include "FinalizerContext.h"

#if STORM_GC == STORM_GC_SMM

namespace storm {
	namespace smm {

		FinalizerContext::FinalizerContext() : shared(null) {}

		FinalizerContext::~FinalizerContext() {
			// Tell all threads we're done.
			for (Workers::iterator i = workers.begin(), end = workers.end(); i != end; ++i) {
				i->second->avail.up();
			}

			if (shared) {
				// Performs cleanup as is needed.
				shared->release();
			} else {
				// We need to clean up now.
				if (local.pool) {
					((local.pool)->*(local.poolFn))(local.poolAux);
				}
			}
		}

		void FinalizerContext::finalize(fmt::Obj *object, const os::Thread &thread) {
			Worker *worker;
			Workers::iterator i = workers.find(thread.id());
			if (i == workers.end()) {
				if (!shared) {
					shared = new Finish(local);
				}

				worker = new Worker(shared);
				workers.insert(std::make_pair(thread.id(), worker));

				os::FnCall<void, 2> call = os::fnCall().add(worker);
				os::UThread::spawn(address(&Worker::work), true, call, &thread);
			} else {
				worker = i->second;
			}

			util::Lock::L z(worker->lock);
			worker->finalize.push(object);
			worker->avail.up();
		}

		void FinalizerContext::cleanup(FinalizerPool *pool, FinalizerPoolFn fn, void *aux) {
			if (shared) {
				assert(shared->pool == null, L"Cannot cleanup a finalizer pool multiple times!");
				shared->pool = pool;
				shared->poolFn = fn;
				shared->poolAux = aux;
			} else {
				assert(local.pool == null, L"Cannot cleanup a finalizer pool multiple times!");
				local.pool = pool;
				local.poolFn = fn;
				local.poolAux = aux;
			}
		}

		FinalizerContext::Finish::Finish() : refs(1), pool(null), poolFn(null) {}

		void FinalizerContext::Finish::addRef() {
			atomicIncrement(refs);
		}

		void FinalizerContext::Finish::release() {
			if (atomicDecrement(refs) == 0) {
				// We were last. Clean up!

				if (pool) {
					(pool->*poolFn)(poolAux);
				}

				delete this;
			}
		}

		FinalizerContext::Worker::Worker(Finish *finish) : avail(0), finish(finish) {
			finish->addRef();
		}

		void FinalizerContext::Worker::work() {
			while (true) {
				// Wait for an element.
				avail.down();

				fmt::Obj *obj;
				{
					util::Lock::L z(lock);

					// If empty, we're done.
					if (finalize.empty())
						break;

					obj = finalize.front();
					finalize.pop();
				}

				os::Thread thread = os::Thread::invalid;
				fmt::objFinalize(obj, &thread);

				// Needed if the object was in the nonmoving pool. Doesn't hurt in the other pools.
				fmt::objClearHasFinalizer(obj);

				// We ignore if the finalizer screams again.
#ifdef DEBUG
				if (thread != os::Thread::invalid) {
					WARNING(L"The finalizer for " << obj << L" failed to execute, even when run on another thread.");
				}
#endif
			}

			// Perform cleanup if we were last.
			finish->release();

			delete this;
		}

	}
}

#endif
