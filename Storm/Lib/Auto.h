#pragma once

namespace storm {

	class Object;

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

		// Casting.
		template <class U>
		inline Auto<U> as() {
			U *o = dynamic_cast<U>(obj);
			o->addRef();
			return Auto<U>(o);
		}

		// Return the pointer (releases our ownership of it).
		inline T *ret() { T *t = obj; obj = null; return t; }

		// Get a pointer, borriowing the reference.
		inline T *borrow() const { return obj; }

		// Operators.
		inline T *operator ->() const { return obj; }
		inline T &operator *() const { return *obj; }

		// Check for null.
		inline operator bool() {
			return obj != null;
		}

	private:
		// Owned object.
		T *obj;
	};

}
