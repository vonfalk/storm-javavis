#pragma once
#include "Object.h"
#include "Types.h"
#include "Handle.h"
#include "Storm/Value.h"

namespace storm {

	/**
	 * Array for use in Storm and in C++.
	 */

	// This function is implemented in ArrayTemplate.cpp
	// Look up a specific array type (create it if it is not already created). Implemented in ArrayTemplate.cpp
	Type *arrayType(Engine &e, const Value &type);

	/**
	 * The base class that is used in Storm, use the derived class in C++.
	 */
	class ArrayBase : public Object {
		STORM_CLASS;
	public:
		// Empty array.
		ArrayBase(const Handle &type);

		// Copy another array.
		ArrayBase(const ArrayBase &other);

		// Dtor.
		~ArrayBase();

		// Get size.
		inline Nat STORM_FN count() const { return size; }

		// Clear.
		void STORM_FN clear();

		// Raw operations.
		void pushRaw(const void *value) {
			ensure(size + 1);
			(*handle.create)(ptr(size++), value);
		}

		void *atRaw(nat id) {
			assert(id < size);
			return ptr(id);
		}

	protected:
		// Handle
		const Handle &handle;

		// Size.
		nat size;

		// Capacity
		nat capacity;

		// Allocations.
		byte *data;

		// Get the pointer to an element.
		inline void *ptr(nat id) { return data + (id * handle.size); }

		// Ensure 'data' can hold at least 'n' objects.
		void ensure(nat n);

		// Destroy previously allocated space.
		void destroy(byte *data, nat elements);
	};


	template <class T>
	class Array : public ArrayBase {
		TYPE_EXTRA_CODE
	public:
		static Type *type(Engine &e) { return arrayType(e, value<T>(e)); }
		static Type *type(const Object *o) { return arrayType(o->engine(), value<T>(o->engine())); }

		// Empty array.
		Array() : ArrayBase(storm::handle<T>()) {}

		// Copy array.
		Array(const Array<T> &o) : ArrayBase(o) {}

		// Element access.
		T &at(Nat i) {
			assert(i < size);
			return ((T *)data)[i];
		}

		const T &at(Nat i) const {
			assert(i < size);
			return ((T *)data)[i];
		}

		// Insert an element.
		void push(const T &item) {
			pushRaw(&item);
		}

	protected:
		virtual void output(wostream &to) const {
			to << L"[";

			if (count() > 0)
				to << ::toS(at(0));

			for (nat i = 1; i < count(); i++)
				to << L", " << ::toS(at(i));

			to << L"]";
		}

	};

	/**
	 * Array of pointers using Auto. Synonym with Array<Auto<T>>.
	 */
	template <class T>
	class ArrayP : public Array<Auto<T> > {
	public:

		// Empty array.
		ArrayP() : Array() {}

		// Copy array.
		ArrayP(const ArrayP<T> &o) : ArrayBase(o) {}
	};

}
