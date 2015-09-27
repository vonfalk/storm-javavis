#pragma once
#include "StrBuf.h"
#include "Hash.h"
#include "Utils/Templates.h"
#include "Utils/DetectOperator.h"

CREATE_DETECTOR(deepCopy);
CREATE_DETECTOR(hash);
CREATE_DETECTOR(equals);

namespace storm {
	class Str;
	class StrBuf;
	class CloneEnv;

	/**
	 * A type handle, ie information about a type without actually knowing
	 * exactly what the type itself is.
	 * Used to make it easier to implement templates usable in Storm from C++.
	 */
	class Handle : NoCopy {
	public:
		// Size of the type.
		size_t size;

		// Destructor (may be null).
		typedef void (CODECALL *Destroy)(void *object);
		Destroy destroy;

		// Copy-construct (to, from)
		typedef void (CODECALL *Create)(void *to, const void *from);
		Create create;

		// Deep-copy.
		typedef void (CODECALL *DeepCopy)(void *object, CloneEnv *env);
		DeepCopy deepCopy;

		// Equals.
		typedef bool (CODECALL *Equals)(const void *object, const void *eq);
		Equals equals;

		// Hash.
		typedef nat (CODECALL *Hash)(const void *object);
		Hash hash;

		// Output to StrBuf.
		virtual void output(const void *object, StrBuf *to) const = 0;

		// Floating point value?
		bool isFloat;

		inline Handle() :
			size(0), destroy(null), create(null), deepCopy(null), equals(null), hash(null), isFloat(false) {}
	};

	/**
	 * The handle used when we instantiate it from the handle<T> function below. This is templated,
	 * since we need some information about the type to be able to retrieve the toS function among
	 * others.
	 */
	template <class T>
	class StaticHandle : public Handle {
	public:

		StaticHandle(size_t size, Destroy destroy, Create create, DeepCopy deep, Equals equals, Hash hash, bool isFloat) {
			this->size = size;
			this->destroy = destroy;
			this->create = create;
			this->deepCopy = deep;
			this->equals = equals;
			this->hash = hash;
			this->isFloat = isFloat;
		}

		// Output.
		virtual void output(const void *object, StrBuf *to) const {
			out.output((const T *)object, to);
		}

	private:
		// Helper for the output.
		template <bool object, bool direct>
		struct Output {
			// Use the compiler-generated Handle for this functionality.
			mutable const Handle *handle;

			Output() : handle(null) {}

			void output(const T *obj, StrBuf *to) const {
				if (!handle) {
					// Note: we can not get type info from 'obj', it is probably a value.
					Type *t = stormType<T>(to->engine());
					// Void.
					if (!t)
						return;
					handle = &typeHandle(stormType<T>(to->engine()));
				}

				handle->output(obj, to);
			}
		};

		// Prefer using the StrBuf directly if possible.
		template <bool object>
		struct Output<object, true> {
			void output(const T *obj, StrBuf *to) const {
				steal(to->add(*obj));
			}
		};

		template <>
		struct Output<true, false> {
			void output(const T *obj, StrBuf *to) const {
				// T is Auto<T> or Par<T>, which means it should have a ->toS that is reasonable to use!
				steal(to->add(steal((*obj)->toS())));
			}
		};

		// Any state needed by the output.
		Output<IsAuto<T>::v, CanOutput<T>::v> out;

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
			PLN(L"Found something comparable with ==!");
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

	template <>
	class HandleHashHelper<Byte, false> {
	public:
		static nat CODECALL call(const Byte *v) { return byteHash(*v); }
		static Handle::Hash hash() {
			return (Handle::Hash)&HandleHashHelper<Byte, false>::call;
		}
	};

	template <>
	class HandleHashHelper<Int, false> {
	public:
		static nat CODECALL call(const Int *v) { return intHash(*v); }
		static Handle::Hash hash() {
			return (Handle::Hash)&HandleHashHelper<Int, false>::call;
		}
	};

	template <>
	class HandleHashHelper<Nat, false> {
	public:
		static nat CODECALL call(const Nat *v) { return natHash(*v); }
		static Handle::Hash hash() {
			return (Handle::Hash)&HandleHashHelper<Nat, false>::call;
		}
	};

	template <>
	class HandleHashHelper<Long, false> {
	public:
		static nat CODECALL call(const Long *v) { return longHash(*v); }
		static Handle::Hash hash() {
			return (Handle::Hash)&HandleHashHelper<Long, false>::call;
		}
	};

	template <>
	class HandleHashHelper<Word, false> {
	public:
		static nat CODECALL call(const Word *v) { return wordHash(*v); }
		static Handle::Hash hash() {
			return (Handle::Hash)&HandleHashHelper<Word, false>::call;
		}
	};


	/**
	 * Create handles.
	 */
	template <class T>
	const Handle &handle() {
		static StaticHandle<T> h(
			HandleHelper<T>::size(),
			&HandleHelper<T>::destroy,
			&HandleHelper<T>::create,
			HandleDeepHelper<T, detect_deepCopy<T>::value>::deepCopy(),
			HandleEqualsHelper<T, detect_equals<T>::value, detect_operator::equals<T>::value>::equals(),
			HandleHashHelper<T, detect_hash<T>::value>::hash(),
			IsFloat<T>::value
			);
		return h;
	}

}
