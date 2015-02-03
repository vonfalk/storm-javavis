#pragma once

namespace storm {

	class Object;
	class Engine;
	class Type;

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
		inline Par(const Auto<U> &o) : obj(o.borrow()) {}

		// Borrows 'ptr'.
		inline Par(T *obj) : obj(obj) {}

		// Downcasting.
		template <class U>
		inline Par(const Par<U> &from) : obj(from.borrow()) {}

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
		inline T *ret() { T *t = obj; obj = null; return t; }

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

		// Takes ownership from a Par<> (possibly downcasting)
		template <class U>
		inline Auto(const Par<U> &o) : obj(o.ref()) {}

		// Copy.
		inline Auto(const Auto<T> &from) : obj(from.obj) { obj->addRef(); }

		// Downcast.
		template <class U>
		inline Auto(const Auto<U> &from) : obj(from.borrow()) { obj->addRef(); }

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
				Type *expected = U::type(e);
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
	struct Steal {
		Steal(T *v) : v(v) {}
		~Steal() { v->release(); }

		operator T *() { return v; }

		template <class U>
		operator Par<U>() { return Par<U>(v); }

		template <class U>
		operator Auto<U>() { return Auto<U>(capture(v)); }

		T *v;
	};

	template <class T>
	inline Steal<T> steal(T *ptr) { return Steal<T>(ptr); }

}

namespace stdext {

	/**
	 * Enable hash functions for Auto values.
	 */
	template <class T>
	inline size_t hash_value(const storm::Auto<T> &n) {
		return n->hash();
	}

}

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
