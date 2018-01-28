#pragma once
#include "Utils/Templates.h"
#include "Object.h"

namespace storm {
	STORM_PKG(core.lang); // TODO: Other package? If so, update Compiler/RefHandle as well.

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
		STORM_CTOR Handle();

		// Size of the type.
		size_t size;

		// GcType for arrays of the type.
		const GcType *gcArrayType;

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

		// Helper for safe destroying. Zeroes out any used memory so that we do not retain any
		// unneeded objects.
		inline void safeDestroy(void *obj) const {
			if (destroyFn)
				(*destroyFn)(obj);
			memset(obj, 0, size);
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

		// Equality function.
		typedef Bool (*EqualFn)(const void *a, const void *b);
		UNKNOWN(PTR_GC) EqualFn equalFn;

		// Less-than function.
		typedef Bool (*LessFn)(const void *a, const void *b);
		UNKNOWN(PTR_GC) LessFn lessFn;

		// Helper for equality comparison. Works if one of 'equalFn' and 'lessFn' is implemented.
		inline Bool hasEqual() const {
			return equalFn || lessFn;
		}
		inline Bool equal(const void *a, const void *b) const {
			if (equalFn) {
				// Use '==' if possible.
				return (*equalFn)(a, b);
			} else {
				// Otherwise, we can use '!(a < b) && !(b < a)' instead.
				return !(*lessFn)(a, b) && !(*lessFn)(a, b);
			}
		}
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

		// Get the type for T.
		static Type *type(Engine &e) {
			return BaseType<T>::Type::stormType(e);
		}
	};

	template <>
	struct StormInfo<void> {
		static Nat id() {
			return -1;
		}

		static const Handle &handle(Engine &e) {
			return runtime::voidHandle(e);
		}

		static Type *type(Engine &e) {
			return null;
		}
	};

	STORM_PKG(core);

	// Specializations for built-in types. Generates StormInfo for them too, see Storm.h.
	STORM_PRIMITIVE(Bool, createBool);
	STORM_PRIMITIVE(Byte, createByte);
	STORM_PRIMITIVE(Int, createInt);
	STORM_PRIMITIVE(Nat, createNat);
	STORM_PRIMITIVE(Long, createLong);
	STORM_PRIMITIVE(Word, createWord);
	STORM_PRIMITIVE(Float, createFloat);
	STORM_PRIMITIVE(Double, createDouble);

	/**
	 * Helper for figuring out how to create objects.
	 */
	template <class T>
	struct CreateFn {
		// Create a value.
		static void fn(void *to, Engine &e) {
			new (Place(to)) T();
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
