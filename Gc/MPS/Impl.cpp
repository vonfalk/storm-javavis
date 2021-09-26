#include "stdafx.h"
#include "Impl.h"

#if STORM_GC == STORM_GC_MPS

#include "FinalizerQueue.h"
#include "Gc/Code.h"
#include "Gc/Gc.h"
#include "Gc/VTable.h"
#include "Gc/Scan.h"
#include "Gc/Format.h"
#include "Gc/Exception.h"
#include "OS/InlineSet.h"
#include "Core/GcCode.h"
#include "Utils/Memory.h"

// Use debug pools in MPS (behaves slightly differently from the standard and may not trigger errors).
#define MPS_DEBUG_POOL 0

// Check the heap after this many allocations.
#define MPS_CHECK_INTERVAL 100000


namespace storm {

	/**
	 * MPS uses the standard object format in Format.h. See more details there.
	 *
	 * Objects are always allocated in multiples of the alignment, as that is required by the MPS
	 * and it makes it easier to reason about how forwarders and padding behave.
	 *
	 * Convention: Base pointers are always passed as MpsObj * while client pointers are always
	 * passed as either void * or mps_addr_t.
	 *
	 * MPS specific functions generally start with 'mps'.
	 */
	using namespace fmt;


	// Skip objects. Figure out the size of an object, and return a pointer to after the end of it
	// (including the next object's header, it seems).
	static mps_addr_t mpsSkip(mps_addr_t at) {
		return fmt::skip(at);
	}

	// Create a forwarding object at 'at' referring to 'to'. These must be recognized by mpsSize, mpsSkip and mpsScan.
	static void mpsMakeFwd(mps_addr_t at, mps_addr_t to) {
		fmt::makeFwd(at, to);
	}

	// Check if the object is a forwarding object.
	static mps_addr_t mpsIsFwd(mps_addr_t at) {
		return fmt::isFwd(at);
	}

	// Create a padding object. These must be recognized by mpsSize, mpsSkip and mpsScan. Note: Does
	// not work with client pointers.
	static void mpsMakePad(mps_addr_t at, size_t size) {
		fmt::objMakePad((Obj *)at, size);
	}

	/**
	 * Thread description.
	 */
	struct GcThread {
		// # of allocations since last check for finalization messages.
		nat lastFinalization;

		// MPS thread description.
		mps_thr_t thread;

		// Allocation point.
		mps_ap_t ap;

		// Root for this thread.
		mps_root_t root;

		// All threads running on this thread.
		const os::InlineSet<os::Stack> *stacks;

		// Queue of finalizers for this thread.
		FinalizerQueue finalizers;

		// Is there a thread running finalizers?
		size_t runningFinalizers;

#if MPS_CHECK_MEMORY
		// How many allocations ago did we validate memory?
		nat lastCheck;
#endif
	};


	/**
	 * Custom scanning for MPS.
	 *
	 * We emulate the effect of the macro MPS_SCAN_BEGIN here.
	 */
	struct MpsScanner {
		typedef mps_res_t Result;
		typedef mps_ss_t Source;

		// State, as defined in MPS_SCAN_BEGIN.
		mps_ss_t _ss;
		mps_word_t _mps_zs;
		mps_word_t _mps_w;
		mps_word_t _mps_ufs;

		MpsScanner(mps_ss_t ss) : _ss(ss), _mps_zs(ss->_zs), _mps_w(ss->_w), _mps_ufs(ss->_ufs) {}

		~MpsScanner() {
			_ss->_ufs = _mps_ufs;
		}

		inline ScanOption object(void *, void *) const {
			return scanAll;
		}

		inline bool fix1(mps_addr_t ptr) {
			mps_word_t _mps_wt;
			return MPS_FIX1(_ss, ptr);
		}

		inline mps_res_t fix2(mps_addr_t *ptr) {
			return MPS_FIX2(_ss, ptr);
		}

		inline bool fixHeader1(GcType *ptr) {
			mps_word_t _mps_wt;
			return MPS_FIX1(_ss, ptr);
		}

		inline Result fixHeader2(GcType **ptr) {
			return MPS_FIX2(_ss, (void **)ptr);
		}
	};

	static mps_res_t mpsScanExceptions(mps_ss_t ss, void *, size_t) {
		return storm::scanExceptions<MpsScanner>(ss);
	}

	static mps_res_t mpsScan(mps_ss_t ss, mps_addr_t base, mps_addr_t limit) {
		return fmt::Scan<MpsScanner>::objects(ss, base, limit);
	}

	// Stack dummy used to recognize when the MPS wants to scan an entire stack. This is the largest
	// possible address (word aligned). Ie. 0xFF...F0
	static void *const stackDummy = (void *)((size_t)-1 & ~(wordSize - 1));

	// Note: We're checking all word-aligned positions as we need to make sure we're scanning
	// the return addresses into functions (which are also in this pool). MPS currently scans
	// EIP as well, which is good as the currently executing function might otherwise be moved.
	static mps_res_t mpsScanThread(mps_ss_t ss, void *base, void *limit, void *closure) {
		GcThread *thread = (GcThread *)closure;
		mps_res_t r;

		// Scan the finalizer queue as well. It is scanned as ambiguous, even though it is
		// exact. That is fine, however, as it is generally cleared quite quickly.
		storm::Scan<MpsScanner>::array(ss, &thread->finalizers.head, 1);

		if (limit == stackDummy) {
			// We shall scan the entire stack! This is a bit special, as we will more or less
			// completely ignore what MPS told us and figure it out ourselves.
			mps_decrease_scanned(ss, (char *)limit - (char *)base);

			size_t scanned = 0;
			r = storm::Scan<MpsScanner>::stacks(ss, *thread->stacks, base, &scanned);

			// Tell the MPS what we did.
			mps_increase_scanned(ss, scanned);
		} else {
			// Just scan as if it was an array.
			r = storm::Scan<MpsScanner>::array(ss, base, limit);
		}

		return r;
	}

	static mps_res_t mpsScanArray(mps_ss_t ss, void *base, size_t count) {
		return storm::Scan<MpsScanner>::array(ss, base, count);
	}


	// Check return codes from MPS.
	static void check(mps_res_t result, const wchar_t *msg) {
		if (result != MPS_RES_OK)
			throw GcError(msg);
	}

	// Macros usable to increase readability inside 'generationParams'. Note: we measure in KB, not bytes!
#define KB
#define MB *1024

	/**
	 * Parameters about the generations to create. Note: the mortality rate is only an initial
	 * guess, the MPS computes the actual mortality during run-time.
     *
	 * TODO: Tweak these!
	 */
	static mps_gen_param_s generationParams[] = {
		// Nursery generation. Should be fairly small.
		{ 8 MB, 0.9 },

		// Intermediate generation.
		{ 32 MB, 0.5 },

		// Long-lived generation (for types, code and other things).
		{ 128 MB, 0.1 },
	};

	// Remember if we have set up MPS, and synchronize initialization.
	static bool mpsInitDone = false;

	// Set up MPS the first time we try to use it. We only want to call it once, otherwise it will
	// overwrite handlers that the MPS installs for us.
	static void mpsInit() {
		static util::Lock mpsInitLock;
		util::Lock::L z(mpsInitLock);

		if (!mpsInitDone) {
			mpsInitDone = true;
			mps_init();
		}
	}

	GcImpl::GcImpl(size_t initialArena, Nat finalizationInterval) : finalizationInterval(finalizationInterval) {
		// We work under these assumptions.
		fmt::init();
		assert(vtable::allocOffset() >= sizeof(void *), L"Invalid vtable offset (initialization failed?)");

		// Note: This is defined in Gc/mps.c, and only aids in debugging.
		mpsInit();

		MPS_ARGS_BEGIN(args) {
			MPS_ARGS_ADD(args, MPS_KEY_ARENA_SIZE, initialArena);
			check(mps_arena_create_k(&arena, mps_arena_class_vm(), args), L"Failed to create GC arena.");
		} MPS_ARGS_END(args);

		// Note: we can use mps_arena_pause_time_set(arena, 0.0) to enforce minimal pause
		// times. Default is 0.100 s, this should be configurable from Storm somehow.

		check(mps_chain_create(&chain, arena, ARRAY_COUNT(generationParams), generationParams),
			L"Failed to set up generations.");

		MPS_ARGS_BEGIN(args) {
#if MPS_CHECK_MEMORY
			MPS_ARGS_ADD(args, MPS_KEY_FMT_ALIGN, nextPowerOfTwo(headerSize));
#else
			MPS_ARGS_ADD(args, MPS_KEY_FMT_ALIGN, wordSize); // Default alignment.
#endif
			MPS_ARGS_ADD(args, MPS_KEY_FMT_HEADER_SIZE, headerSize);
			MPS_ARGS_ADD(args, MPS_KEY_FMT_SCAN, &mpsScan);
			MPS_ARGS_ADD(args, MPS_KEY_FMT_SKIP, &mpsSkip);
			MPS_ARGS_ADD(args, MPS_KEY_FMT_FWD, &mpsMakeFwd);
			MPS_ARGS_ADD(args, MPS_KEY_FMT_ISFWD, &mpsIsFwd);
			MPS_ARGS_ADD(args, MPS_KEY_FMT_PAD, &mpsMakePad);
			check(mps_fmt_create_k(&format, arena, args), L"Failed to create object format.");
		} MPS_ARGS_END(args);

		MPS_ARGS_BEGIN(args) {
			MPS_ARGS_ADD(args, MPS_KEY_CHAIN, chain);
			MPS_ARGS_ADD(args, MPS_KEY_FORMAT, format);
#if MPS_DEBUG_POOL
			check(mps_pool_create_k(&pool, arena, mps_class_ams_debug(), args), L"Failed to create a GC pool.");
#else
			check(mps_pool_create_k(&pool, arena, mps_class_amc(), args), L"Failed to create a GC pool.");
#endif
		} MPS_ARGS_END(args);

		// GcType-objects are stored in an AMC pool, as those objects are not protected.
		MPS_ARGS_BEGIN(args) {
			MPS_ARGS_ADD(args, MPS_KEY_CHAIN, chain);
			// Store types in the last generation, as they are very long-lived.
			MPS_ARGS_ADD(args, MPS_KEY_GEN, ARRAY_COUNT(generationParams) - 1);
			MPS_ARGS_ADD(args, MPS_KEY_FORMAT, format);
			// We want to support ambiguous references to this pool (eg. from the stack).
			MPS_ARGS_ADD(args, MPS_KEY_AMS_SUPPORT_AMBIGUOUS, true);
			check(mps_pool_create_k(&gcTypePool, arena, mps_class_amc(), args), L"Failed to create a GC pool for types.");
		} MPS_ARGS_END(args);

		check(mps_ap_create_k(&gcTypeAllocPoint, gcTypePool, mps_args_none), L"Failed to create GC type allocation point.");

		// Non-moving pool.
		MPS_ARGS_BEGIN(args) {
			MPS_ARGS_ADD(args, MPS_KEY_CHAIN, chain);
			// Store types in the next to last generation, as they are often long-lived.
			MPS_ARGS_ADD(args, MPS_KEY_GEN, ARRAY_COUNT(generationParams) - 2);
			MPS_ARGS_ADD(args, MPS_KEY_FORMAT, format);
			// We want to support ambiguous references to this pool (eg. from the stack).
			MPS_ARGS_ADD(args, MPS_KEY_AMS_SUPPORT_AMBIGUOUS, true);
			check(mps_pool_create_k(&staticPool, arena, mps_class_ams(), args), L"Failed to create a GC pool for static objects.");
		} MPS_ARGS_END(args);

		check(mps_ap_create_k(&staticAllocPoint, staticPool, mps_args_none), L"Failed to create static allocation point.");

		// Weak references are stored in a separate pool which supports weak references.
		MPS_ARGS_BEGIN(args) {
			MPS_ARGS_ADD(args, MPS_KEY_CHAIN, chain);
			MPS_ARGS_ADD(args, MPS_KEY_FORMAT, format);
			// Store weak tables in the second generation, as they are usually quite long-lived.
			MPS_ARGS_ADD(args, MPS_KEY_GEN, 2);
			check(mps_pool_create_k(&weakPool, arena, mps_class_awl(), args), L"Failed to create a weak GC pool.");
		} MPS_ARGS_END(args);

		MPS_ARGS_BEGIN(args) {
			MPS_ARGS_ADD(args, MPS_KEY_RANK, mps_rank_weak());
			check(mps_ap_create_k(&weakAllocPoint, weakPool, args), L"Failed to create weak allocation point.");
		} MPS_ARGS_END(args);

		// Code allocations.
		MPS_ARGS_BEGIN(args) {
			// TODO: Make code live in its own chain, as code allocations follow very different
			// patterns compared to other data.
			MPS_ARGS_ADD(args, MPS_KEY_CHAIN, chain);
			MPS_ARGS_ADD(args, MPS_KEY_FORMAT, format);
			check(mps_pool_create_k(&codePool, arena, mps_class_amc(), args), L"Failed to create a code GC pool.");
		} MPS_ARGS_END(args);

		check(mps_ap_create_k(&codeAllocPoint, codePool, mps_args_none), L"Failed to create code allocation point.");

#ifdef MPS_USE_IO_POOL
		// Buffer allocations.
		MPS_ARGS_BEGIN(args) {
			MPS_ARGS_ADD(args, MPS_KEY_CHAIN, chain);
			MPS_ARGS_ADD(args, MPS_KEY_GEN, 2);
			MPS_ARGS_ADD(args, MPS_KEY_FORMAT, format);
#if MPS_USE_IO_POOL == 1
			check(mps_pool_create_k(&ioPool, arena, mps_class_lo(), args), L"Failed to create GC pool for buffers.");
#elif MPS_USE_IO_POOL == 2
			check(mps_pool_create_k(&ioPool, arena, mps_class_amcz(), args), L"Failed to create GC pool for buffers.");
#endif
		} MPS_ARGS_END(args);

		check(mps_ap_create_k(&ioAllocPoint, ioPool, mps_args_none), L"Failed to create buffer allocation point.");
#endif

		// We want to receive finalization messages.
		mps_message_type_enable(arena, mps_message_type_finalization());

		// Add a root for exceptions in flight.
		check(mps_root_create(&exRoot, arena, mps_rank_ambig(), (mps_rm_t)0, &mpsScanExceptions, null, 0),
			L"Failed to create a root for the exceptions.");

		// Initialize.
		runningFinalizers = 0;
	}

	void GcImpl::destroy() {
		// Note: All threads are removed by the Gc class, so we can assume no threads are attached.

		mps_root_destroy(exRoot);

		// Collect the entire arena. We have no roots attached, so all objects with finalizers
		// should be found. Leaves the arena in a parked state, so no garbage collections will start
		// after this one.
		mps_arena_collect(arena);

		// See if any finalizers needs to be executed. Here, we should ignore freeing any GcTypes,
		// as the finalization order for the last types are unknown.
		checkFinalizersLocked();

		// Destroy the global allocation points.
		mps_ap_destroy(gcTypeAllocPoint);
		mps_ap_destroy(staticAllocPoint);
		mps_ap_destroy(weakAllocPoint);
		mps_ap_destroy(codeAllocPoint);
#ifdef MPS_USE_IO_POOL
		mps_ap_destroy(ioAllocPoint);
#endif

		// Destroy pools.
		mps_pool_destroy(pool);
		mps_pool_destroy(weakPool);
		mps_pool_destroy(staticPool);
		mps_pool_destroy(codePool);
#ifdef MPS_USE_IO_POOL
		mps_pool_destroy(ioPool);
#endif
		// The type pool has to be destroyed last, as any formatted object (not code) might reference things in here.
		mps_pool_destroy(gcTypePool);

		// Destroy format and chains.
		mps_fmt_destroy(format);
		mps_chain_destroy(chain);
		mps_arena_destroy(arena);
		arena = null;
	}

	MemorySummary GcImpl::summary() {
		TODO(L"Provide a memory summary for MPS!");
		return MemorySummary();
	}

	void GcImpl::collect() {
		mps_arena_collect(arena);
		mps_arena_release(arena);
		// mps_arena_step(arena, 10.0, 1);
		checkFinalizers();
	}

	Bool GcImpl::collect(nat time) {
		TODO(L"Better value for 'multiplier' here?");
		// mps_bool_t != bool...
		return mps_arena_step(arena, time / 1000.0, 1) ? true : false;
	}

	GcThread *GcImpl::attachThread() {
		// Note: We will leak memory if "check" fails. This is rare enough that we don't care.
		GcThread *desc = new GcThread;
		desc->lastFinalization = 0;
		desc->runningFinalizers = 0;

#if MPS_CHECK_MEMORY
		desc->lastCheck = 0;
#endif

		// Register the thread with MPS.
		check(mps_thread_reg(&desc->thread, arena), L"Failed registering a thread with the gc.");

		// Find all stacks on this os-thread.
		desc->stacks = &os::Thread::current().stacks();

		// Register the thread's root. Note that we're fooling MPS on where the stack starts, as we
		// will discover this ourselves later in a platform-specific manner depending on which
		// UThread is currently running.
		check(mps_root_create_thread_scanned(&desc->root,
												arena,
												mps_rank_ambig(),
												(mps_rm_t)0,
												desc->thread,
												&mpsScanThread,
												desc,
												stackDummy),
			L"Failed creating thread root.");

		// Create an allocation point for the thread.
		check(mps_ap_create_k(&desc->ap, pool, mps_args_none), L"Failed to create an allocation point.");

		return desc;
	}

	void GcImpl::detachThread(GcThread *desc) {
		mps_ap_destroy(desc->ap);
		mps_root_destroy(desc->root);
		mps_thread_dereg(desc->thread);
		delete desc;
	}

	// Thread-local variables for remembering the current thread's allocation point. We need some
	// integrity checking to support the (rare) case of one thread allocating from different
	// Engine:s. We do this by remembering which Gc-instance the saved ap is valid for.
	static THREAD GcImpl *currentInfoOwner = null;
	static THREAD GcThread *currentInfo = null;

	mps_ap_t &GcImpl::currentAllocPoint() {
		GcThread *info = null;

		// Check if everything is as we left it. This should be fast as it is called for every allocation!
		// Note: the 'currentInfoOwner' check should be sufficient.
		// Note: we will currently not detect a thread that has been detached too early.
		if (currentInfo != null && currentInfoOwner == this) {
			info = currentInfo;
		} else {
			// Either first time or allocations from another Gc in this thread since last time.
			// This is expected to happen rarely, so it is ok to be a bit slow here.
			GcThread *thread = Gc::threadData(this, os::Thread::current(), null);
			if (!thread)
				throw GcError(L"Trying to allocate memory from a thread not registered with the GC.");

			currentInfo = info = thread;
			currentInfoOwner = this;
		}

		// Shall we run any finalizers right now?
		if (++info->lastFinalization >= finalizationInterval) {
			info->lastFinalization = 0;
			checkFinalizers();
		}

#if MPS_CHECK_MEMORY
		// Shall we validate memory now?
		if (++info->lastCheck >= MPS_CHECK_INTERVAL) {
			info->lastCheck = 0;
			checkMemory();
		}
#endif

		return info->ap;
	}

	void *GcImpl::alloc(const GcType *type) {
		assert(type->kind == GcType::tFixed
			|| type->kind == GcType::tFixedObj
			|| type->kind == GcType::tType, L"Wrong type for calling alloc().");

		size_t size = sizeObj(type);
		mps_ap_t &ap = currentAllocPoint();
		mps_addr_t memory;
		void *result;
		do {
			check(mps_reserve(&memory, ap, size), L"Out of memory (alloc).");
			result = initObj(memory, type, size);
		} while (!mps_commit(ap, memory, size));

		if (type->finalizer)
			mps_finalize(arena, &result);

		return result;
	}

	void *GcImpl::allocStatic(const GcType *type) {
		assert(type->kind == GcType::tFixed
			|| type->kind == GcType::tFixedObj
			|| type->kind == GcType::tType, L"Wrong type for calling allocStatic().");
		// Since we're sharing one allocation point, take the lock for it.
		util::Lock::L z(staticAllocLock);

		size_t size = sizeObj(type);
		mps_addr_t memory;
		void *result;
		do {
			check(mps_reserve(&memory, staticAllocPoint, size), L"Out of memory (static allocation).");
			result = initObj(memory, type, size);
		} while (!mps_commit(staticAllocPoint, memory, size));

		if (type->finalizer)
			mps_finalize(arena, &result);

		return result;
	}

	GcArray<Byte> *GcImpl::allocBuffer(size_t count) {
#ifdef MPS_USE_IO_POOL
		const GcType *type = &byteArrayType;
		size_t size = sizeArray(type, count);

		util::Lock::L z(ioAllocLock);

		mps_addr_t memory;
		void *result;
		do {
			check(mps_reserve(&memory, ioAllocPoint, size), L"Out of memory (alloc buffer).");
			result = initArray(memory, type, size, count);
		} while (!mps_commit(ioAllocPoint, memory, size));

		return (GcArray<Byte> *)result;
#else
		return (GcArray<Byte> *)allocArray(&byteArrayType, count);
#endif
	}

	void *GcImpl::allocArray(const GcType *type, size_t elements) {
		assert(type->kind == GcType::tArray, L"Wrong type for calling allocArray().");

		size_t size = sizeArray(type, elements);
		mps_ap_t &ap = currentAllocPoint();
		mps_addr_t memory;
		void *result;
		do {
			check(mps_reserve(&memory, ap, size), L"Out of memory (alloc array).");
			result = initArray(memory, type, size, elements);
		} while (!mps_commit(ap, memory, size));

		if (type->finalizer)
			mps_finalize(arena, &result);

		return result;
	}

	void *GcImpl::allocWeakArray(const GcType *type, size_t elements) {
		if (elements == 0)
			return null;

		util::Lock::L z(weakAllocLock);

		size_t size = sizeArray(type, elements);
		mps_ap_t &ap = weakAllocPoint;
		mps_addr_t memory;
		void *result;
		do {
			check(mps_reserve(&memory, ap, size), L"Out of memory.");
			result = initWeakArray(memory, type, size, elements);
		} while (!mps_commit(ap, memory, size));

		return result;
	}

	GcType *GcImpl::allocType(GcType::Kind kind, Type *type, size_t stride, size_t entries) {
		size_t size = gcTypeSize(entries) + headerSize;

		mps_addr_t memory;
		GcType *result;
		util::Lock::L z(gcTypeAllocLock);

		do {
			check(mps_reserve(&memory, gcTypeAllocPoint, size), L"Out of memory.");
			result = initGcType(memory, entries);
		} while (!mps_commit(gcTypeAllocPoint, memory, size));

		result->kind = kind;
		result->type = type;
		result->stride = stride;
		result->count = entries;
		return result;
	}

	void GcImpl::freeType(GcType *) {
		// Not needed anymore.
	}

	const GcType *GcImpl::typeOf(const void *mem) {
		const Obj *o = fromClient(mem);
		if (objIsCode(o))
			return null;
		else
			return &(objHeader(o)->obj);
	}

	void GcImpl::switchType(void *mem, const GcType *type) {
		// No checking required, Gc does that for us.
		objSetHeader(fromClient(mem), type);
	}

	void GcImpl::checkFinalizers() {
		if (atomicCAS(runningFinalizers, 0, 1) != 0) {
			// Someone else is already checking finalizers.
			return;
		}

		try {
			checkFinalizersLocked();
		} catch (...) {
			// Clear the flag.
			atomicWrite(runningFinalizers, 0);
			throw;
		}

		// Clear the flag.
		atomicWrite(runningFinalizers, 0);
	}

	void GcImpl::checkFinalizersLocked() {
		mps_message_t message;
		while (mps_message_get(&message, arena, mps_message_type_finalization())) {
			mps_addr_t obj;
			mps_message_finalization_ref(&obj, arena, message);
			mps_message_discard(arena, message);

			finalizeObject(obj);
		}
	}

	// Allocation function used by the finalizer queue.
	static void *finAlloc(void *data, const GcType *type, size_t size) {
		GcImpl *me = (GcImpl *)data;
		return me->allocArray(type, size);
	}

	static void CODECALL runFinalizers(GcThread *data) {
		while (void *obj = data->finalizers.pop()) {
			const GcType *t = GcImpl::typeOf(obj);
			if (t->finalizer) {
				os::Thread thread = os::Thread::invalid;
				(*t->finalizer)(obj, &thread);

				// We ignore if the finalizer screams again.
#ifdef DEBUG
				if (thread != os::Thread::invalid) {
					WARNING(L"The finalizer for " << obj << L" failed to execute, even when run on another thread.");
				}
#endif
			}
		}

		// Done. Signal.
		atomicWrite(data->runningFinalizers, 0);
	}

	void GcImpl::finalizeObject(void *obj) {
		const GcType *t = typeOf(obj);

		if (!t) {
			// A code allocation. Call the finalizer over in the Code lib.
			gccode::finalize(obj);
		} else if (t->finalizer) {
			// If it is a tFixedObject, make sure it has been properly initialized before we're trying to destroy it!
			bool finalize = true;
			if (t->kind == GcType::tFixedObj || t->kind == GcType::tType)
				finalize = vtable::from((RootObject *)obj) != null;

			if (finalize) {
				// Mark the object as destroyed so that we can detect it later.
				objSetFinalized(fromClient(obj));

				os::Thread thread = os::Thread::invalid;
				(*t->finalizer)(obj, &thread);

				// Need to execute on another thread?
				if (thread != os::Thread::invalid) {
					GcThread *data = Gc::threadData(this, thread, null);
					if (!data)
						throw GcError(L"Attempting to finalize on a thread not registered with the GC!");

					data->finalizers.push(&finAlloc, this, obj);
					// Note: This is fine since we know only we may start threads at the moment, and we hold a lock.
					if (atomicRead(data->runningFinalizers) == 0) {
						atomicWrite(data->runningFinalizers, 1);
						os::FnCall<void, 2> call = os::fnCall().add(data);
						os::UThread::spawn(address(&runFinalizers), false, call, &thread);
					}
				}
			}
		}
	}

	bool GcImpl::liveObject(RootObject *o) {
		// See if we finalized the object. If so, it is not live anymore.
		return o && !isFinalized(o);
	}

	void *GcImpl::allocCode(size_t code, size_t refs) {
		size_t size = sizeCode(code, refs);
		dbg_assert(size > headerSize, L"Can not allocate zero-sized chunks of code!");

		mps_addr_t memory;
		void *result;
		util::Lock::L z(codeAllocLock);

		do {
			check(mps_reserve(&memory, codeAllocPoint, size), L"Out of memory.");
			result = initCode(memory, size, code, refs);
		} while (!mps_commit(codeAllocPoint, memory, size));

		// Register for finalization if the backend asks us to.
		if (gccode::needFinalization())
			mps_finalize(arena, &result);

		return result;
	}

	size_t GcImpl::codeSize(const void *alloc) {
		const Obj *o = fromClient(alloc);
		if (objIsCode(o)) {
			return objCodeSize(o);
		} else {
			dbg_assert(false, L"Attempting to get the size of a non-code block.");
			return 0;
		}
	}

	GcCode *GcImpl::codeRefs(void *alloc) {
		return refsCode(fromClient(alloc));
	}

	void GcImpl::startRamp() {
		check(mps_ap_alloc_pattern_begin(currentAllocPoint(), mps_alloc_pattern_ramp()), L"RAMP");
	}

	void GcImpl::endRamp() {
		check(mps_ap_alloc_pattern_end(currentAllocPoint(), mps_alloc_pattern_ramp()), L"RAMP");
	}

	struct WalkData {
		mps_fmt_t fmt;
		GcImpl::WalkCb fn;
		void *data;
	};

	static void walkFn(mps_addr_t addr, mps_fmt_t fmt, mps_pool_t pool, void *p, size_t s) {
		WalkData *d = (WalkData *)p;
		if (fmt != d->fmt)
			return;

		const GcType *type = GcImpl::typeOf(addr);
		if (!type)
			return;

		switch (type->kind) {
		case GcType::tFixed:
		case GcType::tFixedObj:
		case GcType::tType:
			// Objects, let them through!
			(*d->fn)((RootObject *)addr, d->data);
			break;
		}
	}

	void GcImpl::walkObjects(WalkCb fn, void *data) {
		mps_arena_park(arena);

		WalkData d = {
			format,
			fn,
			data
		};
		mps_arena_formatted_objects_walk(arena, &walkFn, &d, 0);
		mps_arena_release(arena);
	}

	class MpsRoot : public GcRoot {
	public:
		mps_root_t root;
	};

	GcImpl::Root *GcImpl::createRoot(void *data, size_t count, bool ambig) {
		MpsRoot *r = new MpsRoot();
		mps_res_t res = mps_root_create(&r->root,
										arena,
										ambig ? mps_rank_ambig() : mps_rank_exact(),
										(mps_rm_t)0,
										&mpsScanArray,
										data,
										count);

		if (res == MPS_RES_OK)
			return r;

		delete r;
		throw GcError(L"Failed to create a root.");
	}

	void GcImpl::destroyRoot(Root *root) {
		MpsRoot *r = (MpsRoot *)root;
		mps_root_destroy(r->root);
	}

#if MPS_CHECK_MEMORY

	static void checkBarrier(const MpsObj *obj, const byte *start, nat count, byte pattern, const wchar *type) {
		size_t first = MPS_CHECK_BYTES, last = 0;

		for (size_t i = 0; i < MPS_CHECK_BYTES; i++) {
			if (start[i] != pattern) {
				first = min(first, i);
				last = max(last, i);
			}
		}

		dbg_assert(first > last, objInfo(obj)
				+ L" has an invaild " + type + L" barrier in bytes " + ::toS(first) + L" to " + ::toS(last));
	}

	static void checkHeader(const MpsObj *obj) {
		checkBarrier(obj, (byte *)obj->barrier, MPS_CHECK_BYTES, MPS_HEADER_DATA, L"header");
	}

	// Assumes there is a footer.
	static void checkFooter(const MpsObj *obj) {
		size_t size = obj->totalSize;
		checkBarrier(obj, (const byte *)obj + size - MPS_CHECK_BYTES, MPS_CHECK_BYTES, MPS_FOOTER_DATA, L"footer");

		if (objIsCode(obj)) {
			const GcCode *c = mpsRefsCode(obj);
			checkBarrier(obj, (const byte *)c - MPS_CHECK_BYTES, MPS_CHECK_BYTES, MPS_MIDDLE_DATA, L"middle");

			// NOTE: We can not actually check this here, as objects are checked before any FIX
			// operations have been done.
			// dbg_assert(c->reserved != toClient(obj), L"Invalid self-pointer in code segment.");
		}
	}

	static bool hasFooter(const MpsObj *obj) {
		if (objIsCode(obj))
			return true;

		switch (objHeader(obj)->type) {
		case GcType::tFixed:
		case GcType::tFixedObj:
		case GcType::tType:
		case GcType::tArray:
		case GcType::tWeakArray:
			return true;
		}
		return false;
	}

	static void checkSize(const MpsObj *obj) {
		size_t computed = mpsSize(obj);
		size_t expected = obj->totalSize;
		dbg_assert(computed == expected,
				objInfo(obj) + L": Size does not match. Expected " + ::toS(expected)
				+ L", but computed " + ::toS(computed));
	}

	static void checkObject(const MpsObj *obj) {
		checkHeader(obj);
		checkSize(obj);
		if (hasFooter(obj))
			checkFooter(obj);
	}

	static void initObject(MpsObj *obj, size_t size) {
		// if (obj->allocId == 1) DebugBreak();
		obj->totalSize = size;
		obj->allocId = allocId++;
		memset(obj->barrier, MPS_HEADER_DATA, MPS_CHECK_BYTES);
		if (hasFooter(obj))
			memset((byte *)obj + size - MPS_CHECK_BYTES, MPS_FOOTER_DATA, MPS_CHECK_BYTES);
		if (objIsCode(obj))
			memset((byte *)mpsRefsCode(obj) - MPS_CHECK_BYTES, MPS_MIDDLE_DATA, MPS_CHECK_BYTES);
	}

	void GcImpl::checkPoolMemory(const void *object, bool recursive) {
		object = (const byte *)object - headerSize;
		const MpsObj *obj = (const MpsObj *)object;
		const MpsHeader *header = objHeader(obj);

		checkHeader(obj);

		if (objIsCode(obj)) {
			checkSize(obj);
			if (hasFooter(obj))
				checkFooter(obj);
			return;
		} else {
			mps_pool_t headerPool = null;
			if (mps_addr_pool(&headerPool, arena, (void *)header)) {
				dbg_assert(headerPool == gcTypePool, objInfo(obj)
						+ toHex(object) + L" has an invalid header: " + toHex(header));
			} else {
				// We do support statically allocated object descriptions.
				dbg_assert(readable(header), objInfo(obj)
						+ L" has an invalid header: " + toHex(header));
			}
		}

		checkSize(obj);
		if (hasFooter(obj))
			checkFooter(obj);

		switch (header->type) {
		case GcType::tFixed:
		case GcType::tFixedObj:
		case GcType::tType:
			if (recursive) {
				// Check pointers as well.
				for (nat i = 0; i < header->obj.count; i++) {
					size_t offset = header->obj.offset[i] + headerSize;
					mps_addr_t *data = (mps_addr_t *)((const byte *)obj + offset);
					checkMemory(*data, false);
				}
			}
			break;
		case GcType::tArray:
		case GcType::tWeakArray:
			break;
		case mpsPad0:
		case mpsPad:
		case mpsFwd1:
		case mpsFwd:
			// No validation to do.
			break;
		default:
			dbg_assert(false, objInfo(obj) + L" has an unknown GcType!");
		}
	}

	// Called by MPS for every object on the heap. This function may not call the MPS, nor access
	// other memory managed by the MPS (except for non-protecting pools such as our GcType-pool).
	static void checkObject(mps_addr_t addr, mps_fmt_t fmt, mps_pool_t pool, void *p, size_t) {
		Gc *me = (Gc *)p;

		if (p == me->pool || p == me->typePool || p == me->codePool || p == me->weakPool)
			me->checkPoolMemory(addr, true);
#ifdef MPS_USE_IO_POOL
		if (p == me->ioPool)
			me->checkPoolMemory(addr, true);
#endif
	}

	void GcImpl::checkMemory(const void *object, bool recursive) {
		mps_addr_t obj = (mps_addr_t)object;
		mps_pool_t p;
		mps_addr_pool(&p, arena, obj);

		if (p == pool || p == typePool || p == codePool || p == weakPool)
			checkPoolMemory(obj, recursive);
#ifdef MPS_USE_IO_POOL
		if (p == ioPool)
			checkPoolMemory(obj, recursive);
#endif
	}

	void GcImpl::checkMemory() {
		mps_pool_check_fenceposts(pool);
		mps_pool_check_free_space(pool);
		mps_arena_park(arena);
		mps_arena_formatted_objects_walk(arena, &checkObject, this, 0);
		mps_arena_release(arena);
	}

	void GcImpl::checkMemoryCollect() {
		mps_pool_check_fenceposts(pool);
		mps_pool_check_free_space(pool);

		mps_arena_collect(arena);
		mps_arena_formatted_objects_walk(arena, &checkObject, this, 0);
		mps_arena_release(arena);
	}

#else
	void GcImpl::checkPoolMemory(const void *object, bool recursive) {
		// Nothing to do.
	}

	void GcImpl::checkMemory(const void *, bool) {
		// Nothing to do.
	}

	void GcImpl::checkMemory() {
		// Nothing to do.
	}

	void GcImpl::checkMemoryCollect() {
		// Nothing to do.
	}
#endif

	void GcImpl::dbg_dump() {}

	class MpsGcWatch : public GcWatch {
	public:
		MpsGcWatch(GcImpl &gc) : gc(gc) {
			mps_ld_reset(&ld, gc.arena);
		}

		MpsGcWatch(GcImpl &gc, const mps_ld_s &ld) : gc(gc), ld(ld) {}

		virtual void add(const void *addr) {
			mps_ld_add(&ld, gc.arena, (mps_addr_t)addr);
		}

		virtual void remove(const void *addr) {
			// Not supported by the MPS. Ignore it, as it will 'just' generate false positives, not
			// break anything.
		}

		virtual void clear() {
			mps_ld_reset(&ld, gc.arena);
		}

		virtual bool moved() {
			return mps_ld_isstale_any(&ld, gc.arena) ? true : false;
		}

		virtual bool moved(const void *addr) {
			return mps_ld_isstale(&ld, gc.arena, (mps_addr_t)addr) ? true : false;
		}

		virtual GcWatch *clone() const {
			return new (gc.alloc(&type)) MpsGcWatch(gc, ld);
		}

		static const GcType type;

	private:
		GcImpl &gc;
		mps_ld_s ld;
	};

	const GcType MpsGcWatch::type = {
		GcType::tFixed,
		null,
		null,
		sizeof(MpsGcWatch),
		0,
		{},
	};

	GcWatch *GcImpl::createWatch() {
		return new (alloc(&MpsGcWatch::type)) MpsGcWatch(*this);
	}

	static const GcLicense mpsLicense = {
		S("MPS"),
		S("BSD 2-clause license"),
		S("Ravenbrook Limited"),
		S("Copyright \u00A9")S(" 2001\u2013")S("2020 Ravenbrook Limited (http://www.ravenbrook.com/)\n")
		S("\n")
		S("Redistribution and use in source and binary forms, with or without modification,\n")
		S("are permitted provided that the following conditions are met:\n")
		S("\n")
		S("1. Redistributions of source code must retain the above copyright notice, this\n")
		S("list of conditions and the following disclaimer.\n")
		S("\n")
		S("2. Redistributions in binary form must reproduce the above copyright notice,\n")
		S("this list of conditions and the following disclaimer in the documentation and/or\n")
		S("other materials provided with the distribution.\n")
		S("\n")
		S("THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS\n")
		S("\"AS IS\" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT\n")
		S("LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR\n")
		S("A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT\n")
		S("HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,\n")
		S("SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT\n")
		S("LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,\n")
		S("DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY\n")
		S("THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT\n")
		S("(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE\n")
		S("OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.\n")
	};

	const GcLicense *GcImpl::license() {
		return &mpsLicense;
	}

}

#endif
