#pragma once
#include "Utils/Templates.h"

namespace storm {
	STORM_PKG(core.lang); // TODO: Other package?

	/**
	 * A type handle, ie. information about a type without actually knowing exactly which type it
	 * is. Used to make it easier to implement templates usable in Storm from C++.
	 *
	 * Note: we can always move objects with a memcpy (as this is what the gc does all the time). We
	 * may, however, need to make copies using the copy constructor.
	 *
	 * Note: all function pointers are exported to the GC, as they may point to generated code (which is gc:d).
	 */
	class Handle : public Object {
		STORM_CLASS;
	public:
		// Size of the type.
		size_t size;

		// GcType for arrays of the type.
		GcType *gcArrayType;

		// Copy constructor. Acts as an assignment (ie. never deeply copies heap-allocated types).
		typedef void (*CopyFn)(void *dest, const void *src);
		UNKNOWN(PTR_GC) CopyFn copyFn;

		// Deep copy an instance of this type.
		typedef void (*DeepCopyFn)(void *obj, CloneEnv *env);
		UNKNOWN(PTR_GC) DeepCopyFn deepCopyFn;

		// ToS implementation.
		typedef void (*ToSFn)(void *obj, StrBuf *to);
		UNKNOWN(PTR_GC) ToSFn toSFn;
	};

	// Get a handle for T. (T may be pointer or reference).
	template <class T>
	const Handle &handle(Engine &e) {
		return BaseType<T>::Type::stormHandle(e);
	}
}
