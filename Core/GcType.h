#pragma once

namespace storm {

	// Alignment required for GcType instances.
	// If the garbage collector being used uses the default object format, alignment needs to be
	// at least 8, otherwise it will not function properly. Therefore, we always align GcType
	// instances to 8. On X86-64, this is no difference from the default alignment, and on X86
	// we only waste 4 bytes occasionally (when "count" is even).
#define GCTYPE_ALIGNMENT 8

	/**
	 * Description of a type from the view of the garbage collector. In general, we maintain one
	 * GcType for each Type in the system. A GcType is different from a Type as it only describes
	 * the part of the memory layout relevant to the garbage collector (ie. pointer offsets). It
	 * also does this in a flattened format, so that scanning is fast.
	 *
	 * Make sure to allocate these through the garbage collector interface or statically, as these
	 * might need to be kept separate from other data to ensure the garbage collector will work
	 * properly.
	 *
	 * TODO: Revise the sizes of data types used on 64-bit systems. Some sizes and offsets can be
	 * stored as 32-bit numbers instead of 64-bit numbers to save space.
	 */
	struct ALIGN_AS(GCTYPE_ALIGNMENT) GcType {
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

		// Any finalizer to be run when this type is not reachable any more.
		// NOTE: this pointer is *not* scanned, so it can not point to any generated code at the moment!
		const void *finalizer;

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
