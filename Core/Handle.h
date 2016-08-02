#pragma once

namespace storm {

	/**
	 * A type handle, ie. information about a type without actually knowing exactly which type it
	 * is. Used to make it easier to implement templates usable in Storm from C++.
	 *
	 * Note: we can always move objects with a memcpy (as this is what the gc does all the time). We
	 * may, however, need to make copies using the copy constructor.
	 */
	class Handle {
	public:
		// Size of the type.
		size_t size;

		// GcType for arrays and dynamic arrays of the type.
		GcType *gcArrayType;
		GcType *gcDynArrayType;

		// Copy constructor.
		typedef void (*CopyFn)(void *dest, const void *src);
		CopyFn copyFn;

		// Deep copy an instance of this type.
		typedef void (*DeepCopyFn)(void *obj, CloneEnv *env);
		DeepCopyFn deepCopyFn;

		// ToS implementation.
		typedef void (*ToSFn)(void *obj, StrBuf *to);
		ToSFn toSFn;
	};

}
