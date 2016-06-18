#pragma once
#include "Utils/Exception.h"

#include "gc/mps.h"

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
	 * Description of a type from the view of the garbage collector. In general, we maintain one
	 * GcType for each Type in the system. A GcType is different from a Type as it only describes
	 * the part of the memory layout relevant to the garbage collector (ie. pointer offsets). It
	 * also does this in a flattened format, so that scanning is fast.
	 *
	 * Make sure to allocate these through the garbage collector interface, as these might need to
	 * be kept separate from other data to ensure the garbage collector will work properly.
	 */
	struct GcType {
		// Possible variations in the description.
		enum Kind {
			// A type that has a fixed layout and size (ie. classes, structs).
			tFixed,

			// A variant of tFixed, where offset[0] points to a GcType to be scanned. Used by the
			// Type type to properly scan its GcType member.
			tType,

			// Repeated occurence of another type (ie. an array). The number of repetitions is stored as
			// a size_t in the first element of the allocation (to keep alignment), followed by a number
			// of repeated fixed size allocations.
			tArray,
		};

		// Type. size_t since we want to know the size properly. Has to be first.
		size_t kind;

		/**
		 * Other useful data for the rest of the system:
		 */

		// (scanned) reference to the full description of this type.
		Type *type;

		/**
		 * Description of pointer offsets:
		 */

		// Number of bytes to skip for each element.
		size_t stride;

		// Number of offsets here.
		size_t count;

		// Pointer offsets within each element.
		size_t offset[1];
	};

	/**
	 * Interface to the garbage collector. Storm supports multiple garbage collectors, see Storm.h
	 * on how to choose between them. An instance of this class represents an isolated gc arena,
	 * from which we can allocate memory.
	 */
	class Gc : NoCopy {
	public:
		// Create, gives an initial arena size. This is only an estimate that may be disregarded by
		// the gc.
		Gc(size_t initialArena);

		// Destroy.
		~Gc();

		// Allocate an object of a specific type. Assumes type->type == tFixed.
		void *alloc(const GcType *type);

		// Allocate an array of objects. Assumes type->type == tArray.
		void *allocArray(const GcType *type, size_t count);

		/**
		 * Management of Gc types.
		 */

		// Allocate a gc type.
		GcType *allocType(size_t entries);

		// Deallocate a gc type.
		void freeType(GcType *type);

		// Get the gc type of an allocation.
		static const GcType *allocType(const void *mem);

		/**
		 * Debugging/testing.
		 */

		// Stress-test the gc by allocating a large number of objects.
		void test();

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

		// Allocation point for main thread (TODO: make sure to give each thread its own allocation point)
		mps_ap_t allocPoint;

		// Main thread's root.
		mps_root_t mainRoot;
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
