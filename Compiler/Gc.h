#pragma once
#include "Utils/Exception.h"
#include "Utils/Lock.h"
#include "OS/Thread.h"

#include "gc/mps.h"

#include "Core/GcArray.h"
#include "Core/GcType.h"
#include "Core/GcWatch.h"

namespace storm {

	class Type;

	/**
	 * This file contains the interface to the garbage collector. To reliably run a GC, we need to
	 * be able to describe where each type hides its pointers, and possibly what finalizers to
	 * run.
	 *
	 * Note: Any types describing data layout should be allocated from the corresponding Gc class,
	 * as special care might need to be taken not to interfere with the garbage collector.
	 *
	 * TODO: Revise the sizes of data types used on 64-bit systems. Some sizes and offsets can be
	 * stored as 32-bit numbers instead of 64-bit numbers to save space.
	 */


	/**
	 * Internal description of thread-local data for the garbage collector.
	 */
	struct GcThread;

	/**
	 * Interface to the garbage collector. Storm supports multiple garbage collectors, see Storm.h
	 * on how to choose between them. An instance of this class represents an isolated gc arena,
	 * from which we can allocate memory.
	 */
	class Gc : NoCopy {
	public:
		// Create.
		// 'initialArena' - an initial estimate of the arena size. May be disregarded by the gc if needed.
		// 'finalizationInterval' - how seldom the gc should check for finalizations. An interval of 500 means
		//                          every 500 allocations.
		Gc(size_t initialArena, nat finalizationInterval);

		// Destroy.
		~Gc();

		/**
		 * Manual garbage collection hints.
		 */

		// Do a full garbage collection now.
		void collect();

		// Spend approx 'time' ms on an incremental collection if possible. Returns true if there is more to do.
		bool collect(nat time);

		// TODO: Add interface for managing pause times and getting information about allocations.

		/**
		 * Thread management.
		 */

		// Register the current thread for use with the gc.
		void attachThread();

		// Re-register a thread for use with the gc. Can not be used for new threads.
		void reattachThread(const os::Thread &thread);

		// Unregister a thread from the gc. This has to be done before the thread is destroyed.
		void detachThread(const os::Thread &thread);

		/**
		 * Memory allocation.
		 */

		// Allocate an object of a specific type. Assumes type->type == tFixed.
		void *alloc(const GcType *type);

		// Allocate an array of objects. Assumes type->type == tArray.
		void *allocArray(const GcType *type, size_t count);

		/**
		 * Management of Gc types.
		 *
		 * These objects are semi-managed by the GC. They are not collected automatically, but they
		 * are destroyed together with the GC object. That means, small leaks of GcType objects are
		 * not a problem as long as they don't grow over time.
		 *
		 * TODO: Make these automatically managed in a zero-rank pool?
		 */

		// Allocate a gc type.
		GcType *allocType(GcType::Kind kind, Type *type, size_t stride, size_t entries);
		GcType *allocType(const GcType *original);

		// Deallocate a gc type.
		void freeType(GcType *type);

		// Get the gc type of an allocation.
		static const GcType *typeOf(const void *mem);

		// Change the gc type of an allocation. Used during startup to change header from a fake one
		// (to allow basic scanning) to a real one (with a proper Type) when startup has progressed
		// further.
		static void switchType(void *mem, const GcType *to);

		/**
		 * Roots.
		 */

		struct Root;

		// Allocate a root that scans an array of pointers.
		Root *createRoot(void *data, size_t count);

		// Destroy a root.
		void destroyRoot(Root *root);

		/**
		 * Watch object.
		 */

		GcWatch *createWatch();

		/**
		 * Debugging/testing.
		 */

		// Stress-test the gc by allocating a large number of objects.
		bool test(nat times = 100);

	private:
		// Finalization interval.
		nat finalizationInterval;

		// Internal variables which are implementation-specific:
#ifdef STORM_GC_MPS
		friend class MpsGcWatch;

		// Current arena, pool and format.
		mps_arena_t arena;
		mps_pool_t pool;
		mps_fmt_t format;
		mps_chain_t chain;

		// Separate non-protected pool for GcType objects.
		mps_pool_t gcTypePool;

		// Separate non-moving pool for storing Type-objects. Note: we only have one allocation
		// point for the types since they are rarely allocated.
		mps_pool_t typePool;
		mps_ap_t typeAllocPoint;
		util::Lock typeAllocLock;

		// Description of all attached threads.
		typedef map<uintptr_t, GcThread *> ThreadMap;
		ThreadMap threads;

		// Lock for manipulating the attached threads.
		util::Lock threadLock;

		// Allocate an object in the Type pool.
		void *allocTypeObj(const GcType *type);

		// Attach/detach thread. Sets up/tears down all members in Thread, nothing else.
		void attach(GcThread *thread, const os::Thread &oThread);
		void detach(GcThread *thread);

		// Find the allocation point for the current thread. May run finalizers.
		mps_ap_t &currentAllocPoint();

		// See if there are any finalizers to run at the moment.
		void checkFinalizers();

		// Second stage. Assumes we have exclusive access to the message stream.
		void checkFinalizersLocked();

		// Finalize an object.
		void finalizeObject(void *obj);

		// Are we running finalizers at the moment? Used for synchronization.
		volatile nat runningFinalizers;

		// During destruction - ignore any freeType() calls?
		bool ignoreFreeType;
#endif
	};


	/**
	 * Errors thrown when interacting with the GC.
	 */
	class GcError : public Exception {
	public:
		GcError(const String &msg) : msg(msg) {}
		virtual String what() const { return msg; }
	private:
		String msg;
	};

}
