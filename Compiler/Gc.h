#pragma once
#include "Utils/Exception.h"
#include "Utils/Lock.h"
#include "Utils/InlineSet.h"
#include "OS/Thread.h"

#include "gc/mps.h"

#include "Core/GcArray.h"
#include "Core/GcType.h"
#include "Core/GcWatch.h"
#include "Core/GcCode.h"

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

		// Allocate an array of weak pointers.
		void *allocWeakArray(size_t count);

		/**
		 * Management of Gc types.
		 *
		 * These objects are semi-managed by the GC. They are not collected automatically, but they
		 * are destroyed together with the GC object. That means, small leaks of GcType objects are
		 * not a problem as long as they don't grow over time.
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
		 * Allocation of code storage. Code allocations are slightly more complex than regular data
		 * allocations. First and foremost, this memory needs to be executable, but it also contains
		 * references to other objects in rather weird formats (for example, relative
		 * offsets). Aside from that, there is also the requirement that we may not have a header on
		 * these objects, even though they are generally different size (so we can not use GcArray,
		 * which stores its size at offset 0).
		 *
		 * To meet these requirements, a code allocation consist of two parts: one code part and one
		 * metadata part. The code part is first, and at some later point (the gc keeps track of
		 * this), the metadata block begins. Here, information about all references in the current
		 * block are stored.
		 */

		// Allocate a code block with 'code' bytes of machine code storage and 'refs' entries of reference data.
		void *allocCode(size_t code, size_t refs);

		// Get the size of a code allocation.
		static size_t codeSize(const void *alloc);

		// Access the metadata of a code allocation. Note: all references must be scannable at *any* time.
		static GcCode *codeRefs(void *alloc);

		/**
		 * Iterate through all objects on the heap.
		 *
		 * Note: inside the callback it is only possible to access the object which is passed to the
		 * function and any memory on the stack (this means, for example, it is not possible to use
		 * as<> or call any virtual members in the object). Do not take locks nor attempt to perform
		 * a thread switch inside the callback.
		 */

		// Callback function for a heap walk.
		typedef void (*WalkCb)(RootObject *inspect, void *param);

		// Walk the heap. This usually incurs a full Gc, so it is not a cheap operation.
		void walkObjects(WalkCb fn, void *param);

		/**
		 * Roots.
		 */

		struct Root;

		// Allocate a root that scans an array of pointers.
		Root *createRoot(void *data, size_t count);

		// Destroy a root.
		static void destroyRoot(Root *root);

		/**
		 * Watch object.
		 */

		GcWatch *createWatch();

		/**
		 * Debugging/testing.
		 */

		// Stress-test the gc by allocating a large number of objects.
		bool test(nat times = 100);

		// Check memory consistency. Note: Enable checking in 'Gc.cpp' for this to work.
		void checkMemory();

		// Check consistency of a single object. Note: Enable checking in 'Gc.cpp' for this to work.
		void checkMemory(const void *object);
		void checkMemory(const void *object, bool recursive);

		// Do a gc and check memory collection (sometimes forces memory issues to appear better than
		// just calling 'checkMemory').
		void checkMemoryCollect();

	private:
		// GcType for weak arrays.
		static const GcType weakArrayType;

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

		// Pool for objects with weak references.
		mps_pool_t weakPool;
		mps_ap_t weakAllocPoint;
		util::Lock weakAllocLock;

		// Pool for runnable code. These have their own object format, which is why they are in
		// their own pool.
		mps_fmt_t codeFormat;
		mps_pool_t codePool;
		mps_ap_t codeAllocPoint;
		util::Lock codeAllocLock;

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

		// Struct used for GcTypes inside MPS. As it is not always possible to delete all GcType objects
		// immediatly, we store the freed ones in an InlineSet until we can actually reclaim them.
		struct MpsType : public util::SetMember<MpsType> {
			// Reachable?
			bool reachable;

			// The actual GcType. Must be last, as it contains a 'dynamic' array.
			GcType type;
		};

		// All freed GcType-objects which have not yet been reclaimed.
		util::InlineSet<MpsType> freeTypes;

		// During destruction - ignore any freeType() calls?
		bool ignoreFreeType;

		// Size of an MPS-type.
		static size_t typeSize(size_t offsets);

		// Worker function for 'scanning' the MpsType objects.
		static void markType(mps_addr_t addr, mps_fmt_t fmt, mps_pool_t pool, void *p, size_t);

		// Internal helper for 'checkMemory()'.
		friend void checkObject(mps_addr_t addr, mps_fmt_t fmt, mps_pool_t pool, void *p, size_t);
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
