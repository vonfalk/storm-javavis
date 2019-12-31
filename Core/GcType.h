#pragma once

namespace os {
	class Thread;
}

namespace storm {

	// Finalizer function.
	// The garbage collector executes the finalizer function when an object is about to be
	// collected. If the finalizer needs to be executed on a particular thread, the finalizer
	// needs to check this. If the finalizer was invoked from a thread other than the expected
	// one, then the finalizer should not perform any cleanup and instead set the 'thread'
	// parameter to the thread that should be used instead. The finalizer will then be called
	// again from the correct thread.
	typedef void (CODECALL Finalizer)(void *object, os::Thread *thread);

	/**
	 * Description of a type from the view of the garbage collector. In general, we maintain one
	 * GcType for each Type in the system. A GcType is different from a Type as it only describes
	 * the part of the memory layout relevant to the garbage collector (ie. pointer offsets). It
	 * also does this in a flattened format, so that scanning is fast.
	 *
	 * Make sure to allocate these through the garbage collector interface or statically, as these
	 * might need to be kept separate from other data to ensure the garbage collector will work
	 * properly.
	 */
	struct GcType {
		// Possible variations in the description.
		enum Kind {
			// A type that has a fixed layout and size (ie. classes, structs).
			tFixed,

			// Same as 'tFixed', but offset 0 (ie. the first few bytes in the object) are scanned as
			// a VTable.
			tFixedObj,

			// A variant of tFixedObj, where offset[0] points to a GcType to be scanned. Used by the
			// Type type to properly scan its GcType member.
			tType,

			// Repeated occurence of another type (ie. an array). The number of repetitions is
			// stored as a size_t in the first element of the allocation (to keep alignment) and a
			// user-defined size_t, followed by a number of repeated fixed size allocations. Use
			// GcArray<T> for convenient access.
			tArray,

			// Array of weak pointers. Much like tArray, but with some quirks:
			// - The allocation may *only* contain pointers.
			// - Any non-pointers must be tagged so their pointer-value is not aligned (eg. or with 1).
			// Use GcWeakArray<T> for help with these constraints.
			tWeakArray,
		};

		// Type. size_t since we want to know the size properly. Has to be first.
		size_t kind;

		/**
		 * Other useful data for the rest of the system:
		 */

		// (scanned) reference to the full description of this type. NOTE: Do not change this after
		// the GcType has been created. Doing so may confuse the GC, as this is an unsupported
		// remote reference. Not declared const due to how we are using it.
		Type *type;

		// Finalizer executed when this type is about to be reclaimed.
		// NOTE: This pointer is *not* scanned, so it can not point to code generated at runtime.
		Finalizer *finalizer;

		/**
		 * Description of pointer offsets:
		 */

		// Number of bytes to skip for each element. In the case of tFixed, the size of the allocation.
		size_t stride;

		// Number of offsets here.
		size_t count;

		// Pointer offsets within each element.
		size_t offset[1];
	};

	// Size of a GcType object. Note: Since the last member is an array of 0 elements, the last
	// position is sometimes automatically initialized to zero by C++ (for example, when embedding
	// it in a class that has an automatically generated constructor). In order to avoid
	// accidentally writing outside allocated memory, we pretend that GcType objects containing zero
	// elements actually contain one element. Since there are relatively few GcType objects in the
	// system, this is not a major waste of memory, especially considering this only affects GcType
	// objects which have no pointer offsets.
	inline size_t gcTypeSize(size_t entries) {
		return sizeof(GcType) + max(size_t(1), entries)*sizeof(size_t) - sizeof(size_t);
	}

	// Print a GcType instance, mostly for debugging.
	void printGcType(const GcType *type);
}
