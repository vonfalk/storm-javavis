#pragma once
#include "Utils/Exception.h"

#include "gc/mps.h"

namespace storm {

	/**
	 * Interface to the garbage collector. Storm supports multiple garbage collectors, see Storm.h
	 * on how to choose between them.
	 */
	class Gc : NoCopy {
	public:
		// Create, gives an initial arena size. This is only an estimate that may be disregarded by
		// the gc.
		Gc(size_t initialArena);

		// Destroy.
		~Gc();

	private:
		// Internal variables which are implementation-specific:
#ifdef STORM_GC_MPS
		// Current arena, pool and format.
		mps_arena_t arena;
		mps_pool_t pool;
		mps_fmt_t format;
		mps_chain_t chain;

		// Main thread (TODO: make it better)
		mps_thr_t mainThread;
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
