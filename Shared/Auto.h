#pragma once
#include "Utils/Templates.h"

namespace storm {

#define IF_CONVERTIBLE(From, To) typename EnableIf<IsConvertible<From, To>::value, bool>::t = false

	class Object;
	class Engine;
	class Type;
	class CloneEnv;

	// Copy objects.
	Object *CODECALL cloneObjectEnv(Object *o, CloneEnv *env);


	/**
	 * Calling convention is explained in Object.h
	 */

	template <class T>
	class Auto;

	/**
	 * Automatic reference counting helper. This is a borrowed reference, suitable for
	 * function parameters. It is binary compatible with raw pointers, so that functions
	 * declared with pointers can be called anyway.
	 */
	template <class T>
	class Par {
	public:
		// Create null-ptr
		inline Par() : obj(null) {}

		// Borrow from Auto<> (possibly downcasting)
		template <class U>
		inline Par(const Auto<U> &o, IF_CONVERTIBLE(U, T)) : obj(o.borrow()) {}

		// Borrows 'ptr'.
		inline Par(T *obj) : obj(obj) {}

		// Downcasting.
		template <class U>
		inline Par(const Par<U> &from, IF_CONVERTIBLE(U, T)) : obj(from.borrow()) {}

		// Downcasting (to override the Par(T*) ctor.
		inline Par(const Par<T> &from) : obj(from.borrow()) {}

		// No need for explicit copy, assign or dtor. We do not need to do anything.

		// Upcasting.
		template <class U>
		inline Par<U> as() const {
			U *p = ::as<U>(obj);
			return Par<U>(p);
		}

		// Upcasting, expect a specific type.
		template <class U>
		inline Par<U> expect(Engine &e, const String &context) const {
			Par<U> r = as<U>();
			if (!r) {
				Type *expected = U::type(e);
				Type *got = obj ? obj->myType : null;
				throwTypeError(context, expected, got);
			}
			return r;
		}

		// Return the pointer.
		inline T *ret() { T *t = obj; obj = null; t->addRef(); return t; }

		// Get a pointer, borriowing the reference.
		inline T *borrow() const { return obj; }

		// Get a pointer with a new reference.
		inline T *ref() const { obj->addRef(); return obj; }

		// Operators.
		inline T *operator ->() const { return obj; }
		inline T &operator *() const { return *obj; }

		// Check for null in if-statements.
		inline operator void *() const {
			return obj;
		}

		// Check with other ptr.
		inline bool operator ==(const T *o) {
			return obj == o;
		}

		static Type *stormType(Engine &e) { return T::stormType(e); }
		static Type *stormType(const Object *o) { return T::stormType(o); }
	private:
		// Pointer. Has to be the first and only member.
		T *obj;
	};

	/**
	 * Automatic refcounting of Object-ptrs. Follows the calling convention, and is binary
	 * compatible with a pointer, so that it can be used to receive values from functions
	 * called with raw pointers (breaks calling convention). Not for return values: In that case, use ret().
	 */
	template <class T>
	class Auto {
	public:
		// Create null-ptr
		inline Auto() : obj(null) {}

		// Takes the ownership of 'obj'.
		inline Auto(T *obj) : obj(obj) {}

		// Copy from a Par<> (possibly downcasting)
		template <class U>
		inline Auto(const Par<U> &o, IF_CONVERTIBLE(U, T)) : obj(o.borrow()) { obj->addRef(); }

		// Downcast.
		template <class U>
		inline Auto(const Auto<U> &from, IF_CONVERTIBLE(U, T)) : obj(from.borrow()) { obj->addRef(); }

		// Copy (otherwise the Auto(T *) is ranked higher than the templated one!
		inline Auto(const Auto<T> &from) : obj(from.borrow()) { obj->addRef(); }

		inline Auto<T> &operator =(const Auto<T> &from) {
			obj->release();
			obj = from.obj;
			obj->addRef();
			return *this;
		}

		// Destroy.
		inline ~Auto() { obj->release(); }

		// Upcasting.
		template <class U>
		inline Auto<U> as() const {
			U *o = ::as<U>(obj);
			o->addRef();
			return Auto<U>(o);
		}

		// Upcasting, expect a specific type.
		template <class U>
		inline Auto<U> expect(Engine &e, const String &context) const {
			U *o = ::as<U>(obj);
			if (!o) {
				Type *expected = U::stormType(e);
				Type *got = obj ? obj->myType : null;
				throwTypeError(context, expected, got);
			}
			o->addRef();
			return Auto<U>(o);
		}

		// Return the pointer (releases our ownership of it).
		inline T *ret() const { obj->addRef(); return obj; }

		// Steal the pointer. Sets this to null.
		inline T *steal() { T *t = obj; obj = null; return t; }

		// Get a pointer, borriowing the reference.
		inline T *borrow() const { return obj; }

		// Get a pointer with a new reference.
		inline T *ref() const { obj->addRef(); return obj; }

		// Operators.
		inline T *operator ->() const { return obj; }
		inline T &operator *() const { return *obj; }

		// Check for null in if-statements.
		inline operator void *() const {
			return obj;
		}

		// Check with other ptr.
		inline bool operator ==(const T *o) {
			return obj == o;
		}

		// Deep copy.
		void CODECALL deepCopy(Par<CloneEnv> env) {
			T *n = (T *)cloneObjectEnv(obj, env.borrow());
			obj->release();
			obj = n;
		}

		// Proxies for the type() so that Array<Auto<T>> works.
		static Type *stormType(Engine &e) { return T::stormType(e); }
		static Type *stormType(const Object *o) { return T::stormType(o); }
		template <class Z>
		static Type *stormType(const Auto<Z> &o) { return T::stormType(o); }
		template <class Z>
		static Type *stormType(const Par<Z> &o) { return T::stormType(o); }

	private:
		// Owned object.
		T *obj;
	};

	// Helper to throw an internal type error.
	void throwTypeError(const String &context, Type *expected, Type *got);

	template <class T>
	wostream &operator <<(wostream &to, const Auto<T> &v) {
		T *ptr = v.borrow();
		if (ptr)
			to << *ptr;
		else
			to << L"null";
		return to;
	}

	template <class T>
	wostream &operator <<(wostream &to, const Par<T> &v) {
		T *ptr = v.borrow();
		if (ptr)
			to << *ptr;
		else
			to << L"null";
		return to;
	}

	template <class T>
	Auto<T> capture(T *t) {
		if (t)
			t->addRef();
		return Auto<T>(t);
	}

	/**
	 * Steal a reference into a pointer. Useful when chaining function calls.
	 * foo(steal(bar())) is equivalent to
	 * foo(Auto<T>(bar).borrow())
	 */
	template <class T>
	inline Auto<T> steal(T *ptr) { return Auto<T>(ptr); }


	/**
	 * Template magic to detect Auto or Par.
	 */
	template <class T>
	struct IsAuto {
		static const bool v = false;

		typedef const T &BorrowT;
		static BorrowT borrow(const T &v) { return v; }
	};

	template <class T>
	struct IsAuto<Auto<T>> {
		static const bool v = true;

		typedef T *BorrowT;
		static BorrowT borrow(const Auto<T> &v) { return v.borrow(); }
	};

	template <class T>
	struct IsAuto<Par<T>> {
		static const bool v = true;

		typedef T *BorrowT;
		static BorrowT borrow(const Par<T> &v) { return v.borrow(); }
	};

	/**
	 * Convert Pars to Autos automagically.
	 */
	template <class T>
	struct AsAuto {
		typedef T v;
	};

	template <class T>
	struct AsAuto<Par<T>> {
		typedef Auto<T> v;
	};

	/**
	 * Convert Autos to Pars automagically.
	 */
	template <class T>
	struct AsPar {
		typedef T v;
	};

	template <class T>
	struct AsPar<Auto<T>> {
		typedef Par<T> v;
	};

	// Borrow if possible, otherwise return the same value.
	template <class T>
	typename IsAuto<T>::BorrowT borrow(const T &v) {
		return IsAuto<T>::borrow(v);
	}

}

#ifdef VISUAL_STUDIO
namespace stdext {

	/**
	 * Enable hash functions for Auto values.
	 */
	template <class T>
	inline size_t hash_value(const storm::Auto<T> &n) {
		return n->hash();
	}


	/**
	 * Use the hash value in multimaps. Turns out we need an absolute ordering, and not
	 * just an equality check... This hack will solve it!
	 */
	template <class T>
	class hash_compare<storm::Auto<T> > {
	public:

		// Standard values from the original implementation
		enum {
			bucket_size = 4,
			min_buckets = 8,
		};

		// Ctor.
		hash_compare() {}

		// Hash something
		size_t operator ()(const storm::Auto<T> &k) const {
			// Pseudoranomization found in the original implementation. Probably platform specific.
			long _Quot = k->hash() & LONG_MAX;
			ldiv_t _Qrem = ldiv(_Quot, 127773);

			_Qrem.rem = 16807 * _Qrem.rem - 2836 * _Qrem.quot;
			if (_Qrem.rem < 0)
				_Qrem.rem += LONG_MAX;
			return ((size_t)_Qrem.rem);
		}

		// Ordering
		bool operator ()(const storm::Auto<T> &a, const storm::Auto<T> &b) const {
			if (*a == *b)
				return false;
			// Order by pointers otherwise. A little hacky, but works!
			return a.borrow() < b.borrow();
		}
	};

}
#else
#error "Please define interaction with hash-maps for your compiler!"
#endif

namespace std {

	/**
	 * Specialize for equality checks in maps and so on.
	 */
	template <class T>
	struct equal_to<storm::Auto<T> > {
		bool operator () (const storm::Auto<T> &a, const storm::Auto<T> &b) {
			return (*a) == (*b);
		}
	};

}
