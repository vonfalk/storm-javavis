#pragma once

namespace storm {

	class Object;
	class Engine;
	class Type;

	/**
	 * Automatic refcounting of Object-ptrs. Follows the calling convention, and is binary
	 * compatible with a pointer, so that it can be used to receive values from functions
	 * called with raw pointers. Not for return values. In that case, use ret().
	 */
	template <class T>
	class Auto {
	public:
		// Create null-ptr
		inline Auto() : obj(null) {}

		// Takes the ownership of 'obj'.
		inline Auto(T *obj) : obj(obj) {}

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

		// Downcast to Object.
		inline operator Auto<Object> () {
			return Auto<Object>(obj);
		}

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
		inline T *ret() { T *t = obj; obj = null; return t; }
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
	Auto<T> capture(T *t) {
		if (t)
			t->addRef();
		return Auto<T>(t);
	}
}
