#include "stdafx.h"
#include "Gc.h"

#ifdef STORM_GC_MPS

/**
 * MPS version.
 */
namespace storm {

	/**
	 * Note: in the scanning functions below, we shall not touch any other garbage collected memory,
	 * nor consume too much stack.
	 */

	/**
	 * Size of object.
	 */
	static size_t mpsSize(mps_addr_t base) {
		// TODO!
		return sizeof(void *);
	}

	/**
	 * Skip objects. Figure out the size of an object, and return a pointer to after the end of it.
	 */
	static mps_addr_t mpsSkip(mps_addr_t base) {
		return (char *)base + mpsSize(base);
	}

	/**
	 * Scan objects. If a MPS_FIX returns something other than MPS_RES_OK, return that code as
	 * quickly as possible.
	 */
	static mps_res_t mpsScan(mps_ss_t ss, mps_addr_t base, mps_addr_t limit) {
		MPS_SCAN_BEGIN(ss) {
			for (mps_addr_t at = base; at < limit; at = mpsSkip(at)) {
				// TODO: Scan objects.
			}
		} MPS_SCAN_END(ss);
		return MPS_RES_OK;
	}

	/**
	 * Create a forwarding object at 'at' referring to 'to'. These must be recognized by mpsSize, mpsSkip and mpsScan.
	 */
	static void mpsFwd(mps_addr_t at, mps_addr_t to) {
		// TODO: Implement me!
	}

	/**
	 * See if object at 'at' is a forwarder, and if so, where it points to.
	 */
	static mps_addr_t mpsIsFwd(mps_addr_t at) {
		// TODO: Implement me!
		return null;
	}

	/**
	 * Create a padding object. These must be recognized by mpsSize, mpsSkip and mpsScan.
	 */
	static void mpsPad(mps_addr_t at, size_t size) {
		// TODO: Implement me!
	}


	/**
	 * Check return codes from MPS.
	 */
	static void check(mps_res_t result, const wchar *msg) {
		if (result != MPS_RES_OK)
			throw GcError(msg);
	}

	/**
	 * Parameters about the generations to create.
	 * TODO: Tweak these!
	 */
	static mps_gen_param_s generationParams[] = {
		{ 150, 0.85 },
		{ 170, 0.45 }
	};

	Gc::Gc(size_t arenaSize) {
		MPS_ARGS_BEGIN(args) {
			MPS_ARGS_ADD(args, MPS_KEY_ARENA_SIZE, arenaSize);
			check(mps_arena_create_k(&arena, mps_arena_class_vm(), args), L"Failed to create GC arena.");
		} MPS_ARGS_END(args);

		check(mps_chain_create(&chain, arena, ARRAY_COUNT(generationParams), generationParams),
			L"Failed to set up generations.");

		MPS_ARGS_BEGIN(args) {
			MPS_ARGS_ADD(args, MPS_KEY_FMT_ALIGN, sizeof(void*)); // Default alignment.
			MPS_ARGS_ADD(args, MPS_KEY_FMT_SCAN, &mpsScan);
			MPS_ARGS_ADD(args, MPS_KEY_FMT_SKIP, &mpsSkip);
			MPS_ARGS_ADD(args, MPS_KEY_FMT_FWD, &mpsFwd);
			MPS_ARGS_ADD(args, MPS_KEY_FMT_ISFWD, &mpsIsFwd);
			MPS_ARGS_ADD(args, MPS_KEY_FMT_PAD, &mpsPad);
			check(mps_fmt_create_k(&format, arena, args), L"Failed to create object format.");
		} MPS_ARGS_END(args);

		MPS_ARGS_BEGIN(args) {
			MPS_ARGS_ADD(args, MPS_KEY_CHAIN, chain);
			MPS_ARGS_ADD(args, MPS_KEY_FORMAT, format);
			check(mps_pool_create_k(&pool, arena, mps_class_amc(), args), L"Failed to create a GC pool.");
		} MPS_ARGS_END(args);

		// TODO: Make this better!
		// Register the current thread with MPS.
		check(mps_thread_reg(&mainThread, arena), L"Failed to register thread.");
	}

	Gc::~Gc() {
		// Park the arena, so nothing goes on during cleanup.
		mps_arena_park(arena);

		// TODO: We might want to run a full gc to get destructors properly called.

		// TODO: Fix threads
		mps_thread_dereg(mainThread);

		// Destroy pools.
		mps_pool_destroy(pool);
		mps_fmt_destroy(format);
		mps_chain_destroy(chain);
		mps_arena_destroy(arena);
	}

}

#else

#error "Unsupported or unknown GC chosen. See Storm.h for details."

#endif
