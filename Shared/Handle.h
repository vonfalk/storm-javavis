#pragma once
#include "Utils/Templates.h"

CREATE_DETECTOR(deepCopy);
CREATE_DETECTOR(hash);
CREATE_DETECTOR_MEMBER(operatorEq, operator ==);
CREATE_DETECTOR(equals);

namespace storm {
	class CloneEnv;

	/**
	 * A type handle, ie information about a type without actually knowing
	 * exactly what the type itself is.
	 * Used to make it easier to implement templates usable in Storm from C++.
	 *
	 * NOTE: Do not copy, introducing NoCopy here crashes the static variable below
	 * for some reason...
	 */
	class Handle {
	public:
		// Size of the type.
		size_t size;

		// Destructor (may be null).
		typedef void (CODECALL *Destroy)(void *);
		Destroy destroy;

		// Copy-construct (to, from)
		typedef void (CODECALL *Create)(void *, const void *);
		Create create;

		// Deep-copy.
		typedef void (CODECALL *DeepCopy)(void *, CloneEnv *);
		DeepCopy deepCopy;

		// Equals.
		typedef bool (CODECALL *Equals)(const void *, const void *);
		Equals equals;

		// Hash.
		typedef nat (CODECALL *Hash)(const void *);
		Hash hash;

		// Floating point value?
		bool isFloat;

		inline Handle()
			: size(0), destroy(null), create(null), deepCopy(null), equals(null), hash(null), isFloat(false) {}
		inline Handle(size_t size, Destroy destroy, Create create, DeepCopy deep, Equals equals, Hash hash, bool isFloat)
			: size(size), destroy(destroy), create(create), deepCopy(deep), equals(equals), hash(hash), isFloat(isFloat) {}
	};

	/**
	 * Helper, this allows us to specialize for various types later on!
	 */
	template <class T>
	class HandleHelper {
	public:
		static size_t size() {
			return sizeof(T);
		}

		static void CODECALL destroy(void *v) {
			((T*)v)->~T();
		}

		static void CODECALL create(void *to, const void *from) {
			new (to) T(*((T*)from));
		}

	};

	template <>
	class HandleHelper<void> {
	public:
		static size_t size() {
			return 0;
		}
		static void CODECALL destroy(void *) {}
		static void CODECALL create(void *, const void *) {}
	};

	template <class T, bool hasDeepCopy>
	class HandleDeepHelper {
	public:
		static void CODECALL dummy(void *, CloneEnv *) {}
		static Handle::DeepCopy deepCopy() {
			return (Handle::DeepCopy)&HandleDeepHelper<T, hasDeepCopy>::dummy;
		}
	};

	template <class T>
	class HandleDeepHelper<T, true> {
	public:
		static void CODECALL call(T *o, CloneEnv *env) {
			o->deepCopy(env);
		}
		static Handle::DeepCopy deepCopy() {
			return (Handle::DeepCopy)&HandleDeepHelper<T, true>::call;
		}
	};

	template <class T, bool hasEquals, bool hasOpEq>
	class HandleEqualsHelper {
	public:
		static Handle::Equals equals() { return null; }
	};

	template <class T, bool w>
	class HandleEqualsHelper<T, true, w> {
	public:
		static bool CODECALL eq(const T *a, const T *b) {
			return a->equals(*b);
		}

		static Handle::Equals equals() {
			return (Handle::Equals)&HandleEqualsHelper<T, true, w>::eq;
		}
	};

	// Only use operator == if no equals() is found.
	template <class T>
	class HandleEqualsHelper<T, false, true> {
	public:
		static bool CODECALL eq(const T *a, const T *b) {
			return *a == *b;
		}

		static Handle::Equals equals() {
			return (Handle::Equals)&HandleEqualsHelper<T, false, true>::eq;
		}
	};

	template <class T, bool hasHash>
	class HandleHashHelper {
	public:
		static Handle::Hash hash() { return null; }
	};

	template <class T>
	class HandleHashHelper<T, true> {
	public:
		static nat CODECALL call(const T *v) {
			return v->hash();
		}

		static Handle::Hash hash() {
			return (Handle::Hash)&HandleHashHelper<T, true>::call;
		}
	};

	/**
	 * Create handles.
	 */
	template <class T>
	const Handle &handle() {
		static Handle h = Handle(
			HandleHelper<T>::size(),
			&HandleHelper<T>::destroy,
			&HandleHelper<T>::create,
			HandleDeepHelper<T, detect_deepCopy<T>::value>::deepCopy(),
			HandleEqualsHelper<T, detect_equals<T>::value, detect_operatorEq<T>::value>::equals(),
			HandleHashHelper<T, detect_hash<T>::value>::hash(),
			IsFloat<T>::value
			);
		return h;
	}

}
