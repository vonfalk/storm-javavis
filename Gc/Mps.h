#pragma once

#if STORM_GC == STORM_GC_MPS
#define STORM_HAS_GC

#include "MpsLib.h"

namespace storm {

	// For MPS: use a special non-moving pool for IO buffers. The MPS manual states that the LO pool
	// is suitable for this task as it neither protects nor moves its objects. The AMCZ could also be
	// a decent fit, as it does not protect the objects in the pool and does not move the objects
	// when there is an ambiguous reference to the objects.
	// NOTE: The documentation does not tell wether segmens in LO pools can be 'nailed' by ambiguous
	// pointers to other parts than the start of the object. This is neccessary and provided by AMCZ.
	// Possible values: <undefined>, LO = 1, AMCZ = 2
#define MPS_USE_IO_POOL 2


	class Gc;
	struct GcThread;

	class GcData {
	public:
		// Create.
		GcData(Gc &owner);

		// Destroy ourselves.
		void destroy();

		friend class MpsGcWatch;

		// Current arena, pool and format.
		mps_arena_t arena;
		mps_pool_t pool;
		mps_fmt_t format;
		mps_chain_t chain;

		// Separate non-protected pool for GcType objects.
		mps_pool_t gcTypePool;

		// Separate non-moving pool for storing Type-objects. Note: we only have one allocation
		// point for the types since they are rarely allocated. This pool is also used when
		// interfacing with external libraries which require their objects to not move around.
		mps_pool_t typePool;
		mps_ap_t typeAllocPoint;
		util::Lock typeAllocLock;

		// Pool for objects with weak references.
		mps_pool_t weakPool;
		mps_ap_t weakAllocPoint;
		util::Lock weakAllocLock;

#ifdef MPS_USE_IO_POOL
		// Non-moving, non-protected pool for interaction with foreign code (eg. IO operations in
		// the operating system).
		mps_pool_t ioPool;
		mps_ap_t ioAllocPoint;
		util::Lock ioAllocLock;
#endif

		// Pool for runnable code.
		mps_pool_t codePool;
		mps_ap_t codeAllocPoint;
		util::Lock codeAllocLock;

		// Description of all attached threads.
		typedef map<uintptr_t, GcThread *> ThreadMap;
		ThreadMap threads;

		// Lock for manipulating the attached threads.
		util::Lock threadLock;

		// Finalization interval (our copy).
		nat finalizationInterval;

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
		struct MpsType : public os::SetMember<MpsType> {
			// Constructor. If we don't provide a constructor, 'type' will be default-initialized to
			// zero (from C++11 onwards), which is wasteful. The actual implementation of the
			// constructor will be inlined anyway.
			MpsType();

			// Reachable?
			bool reachable;

			// The actual GcType. Must be last, as it contains a 'dynamic' array.
			GcType type;
		};

		// All freed GcType-objects which have not yet been reclaimed.
		os::InlineSet<MpsType> freeTypes;

		// During destruction - ignore any freeType() calls?
		bool ignoreFreeType;

		// Size of an MpsType.
		static size_t mpsTypeSize(size_t offsets);

		// Worker function for 'scanning' the MpsType objects.
		static void markType(mps_addr_t addr, mps_fmt_t fmt, mps_pool_t pool, void *p, size_t);

		// Internal helper for 'checkMemory()'.
		friend void checkObject(mps_addr_t addr, mps_fmt_t fmt, mps_pool_t pool, void *p, size_t);

		// Check memory, assuming the object is in a GC pool.
		void checkPoolMemory(const void *object, bool recursive);
	};

}

#endif
