#pragma once
#include "Object.h"
#include "Types.h"
#include "Handle.h"

namespace storm {

	/**
	 * Array for use in Storm and in C++.
	 */

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

	protected:
		// Handle
		Handle handle;

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
	public:
		// Empty array.
		Array() : ArrayBase(storm::handle<T>()) {}

		// Copy array.
		Array(const Array<T> &o) : ArrayBase(o) {}

		// Element access.
		T &at(Nat i) {
			assert(i < size);
			return ((T *)data)[i];
		}

		// Insert an element.
		void push(const T &item) {
			ensure(size + 1);
			(*handle.create)(ptr(size++), &item);
		}

	};

}
