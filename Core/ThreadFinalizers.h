#pragma once
#include "Core/GcArray.h"
#include "Utils/Lock.h"

namespace storm {
	STORM_PKG(core.lang);


	/**
	 * A queue of finalizers waiting to be executed on a particular Thread.
	 *
	 * Since it is quite inefficient to allocate a separate UThread for each finalizer that is to be
	 * executed (we might run out of memory in burst finalizations), instead we store finalizers for
	 * other threads in this structure so that they can be executed in a larger batch.
	 *
	 * One instance of this class is created for each thread that need finalizers to be executed at
	 * some point.
	 *
	 * Note: This class is allocated by the garbage collector, but manually provides a GcType. Make
	 * sure to keep the type updated whenever changes are being made to the class layout!
	 *
	 * Note: We don't want to store os::Thread objects in here, as they will not be properly
	 * destroyed (we might end up keeping the thread alive for too long).
	 */
	class ThreadFinalizers {
	public:
		~ThreadFinalizers();

		// Allocate an instance of this object.
		static ThreadFinalizers *create(Engine &e);

		// Finalization function signature.
		typedef void (*DtorFn)(void *);

		// Register an object for finalization on this particular thread. May be called from any thread.
		void finalize(const os::Thread &thread, void *object, DtorFn dtor);

		// Our type description.
		static const GcType gcType;

	private:
		// Create.
		ThreadFinalizers(Engine &e);

		// Number of elements in each chunk.
		static const size_t chunkSize = 200;

		// Engine.
		Engine &e;

		// Are we running a cleanup thread here? Modified using atomics.
		nat cleanupRunning;

		// Convenient representation of elements in the work-list.
		struct Element {
			void *object;
			DtorFn dtor;

			Element() : object(null), dtor(null) {}
			Element(void *object, DtorFn dtor) : object(object), dtor(dtor) {}

			operator bool () const {
				return object != null && dtor != null;
			}
		};

		// Function executed by the cleanup thread.
		void CODECALL cleanup();

		// Lock protecting the data.
		util::Lock lock;

		// List of chunks we're using to keep track of finalized objects.
		// Even indices store objects and odd indices store destructor functions.
		// The last index stores a pointer to the previous chunk.
		GcArray<void *> *work;

		// Push an element to the work-list.
		void push(const Element &elem);

		// Pop an element from the work-list. Returns Elemen() if none exists.
		Element pop();

		// Allocate a chunk.
		GcArray<void *> *allocChunk();
	};

}
