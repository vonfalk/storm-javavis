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

		// Is this type hashed based off its pointer somehow?
		bool locationHash;

		// Copy constructor. Acts as an assignment (ie. never deeply copies heap-allocated types).
		typedef void (*CopyFn)(void *dest, const void *src);
		UNKNOWN(PTR_GC) CopyFn copyFn;

		// Safe copy. Falls back on memcpy.
		inline void safeCopy(void *dest, const void *src) const {
			if (copyFn)
				(*copyFn)(dest, src);
			else
				memcpy(dest, src, size);
		}

		// Destructor. May be null.
		typedef void (*DestroyFn)(void *obj);
		UNKNOWN(PTR_GC) DestroyFn destroyFn;

		// Helper for safe destroying.
		inline void safeDestroy(void *obj) const {
			if (destroyFn)
				(*destroyFn)(obj);
		}

		// Deep copy an instance of this type. May be null.
		typedef void (*DeepCopyFn)(void *obj, CloneEnv *env);
		UNKNOWN(PTR_GC) DeepCopyFn deepCopyFn;

		// ToS implementation.
		typedef void (*ToSFn)(const void *obj, StrBuf *to);
		UNKNOWN(PTR_GC) ToSFn toSFn;

		// Hash function.
		typedef Nat (*HashFn)(const void *obj);
		UNKNOWN(PTR_GC) HashFn hashFn;

		// Equals function.
		typedef Bool (*EqualFn)(const void *a, const void *b);
		UNKNOWN(PTR_GC) EqualFn equalFn;
	};

	/**
	 * Get limited type info for a type (may be pointer or reference).
	 */
	template <class T>
	struct StormInfo {
		// Type id in Storm for this module.
		static Nat id() {
			return BaseType<T>::Type::stormTypeId;
		}

		// Get a handle for T.
		static const Handle &handle(Engine &e) {
			return BaseType<T>::Type::stormHandle(e);
		}
	};

	/**
	 * Helper for figuring out how to create objects.
	 */
	template <class T>
	struct CreateFn {
		// Create a value.
		static void fn(void *to, Engine &e) {
			new (to) T();
		}
	};

	template <class T>
	struct CreateFn<T *> {
		// Create an object or an actor.
		static void fn(void *to, Engine &e) {
			T **o = (T **)to;
			*o = new (e) T();
		}
	};
}
