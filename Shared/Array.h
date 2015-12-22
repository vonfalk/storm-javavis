#pragma once
#include "Object.h"
#include "Types.h"
#include "Handle.h"
#include "Value.h"

namespace storm {
	STORM_PKG(core);

	/**
	 * Array for use in Storm and in C++.
	 */

	// This function is implemented in ArrayTemplate.cpp
	// Look up a specific array type (create it if it is not already created).
	Type *arrayType(Engine &e, const ValueData &type);

	/**
	 * The base class that is used in Storm, use the derived class in C++.
	 */
	class ArrayBase : public Object {
		STORM_CLASS;
	public:
		// Empty array.
		ArrayBase(const Handle &type);

		// Copy another array.
		ArrayBase(Par<ArrayBase> other);

		// Dtor.
		~ArrayBase();

		// Deep copy.
		virtual void STORM_FN deepCopy(Par<CloneEnv> env);

		// Get size.
		inline Nat STORM_FN count() const { return size; }

		// Reserve size.
		void STORM_FN reserve(Nat size);

		// Clear.
		void STORM_FN clear();

		// Any elements?
		inline Bool STORM_FN any() const { return count() > 0; }

		// Empty?
		inline Bool STORM_FN empty() const { return count() == 0; }

		// Erase one element.
		void STORM_FN erase(Nat id);

		// Raw operations.
		void pushRaw(const void *value) {
			ensure(size + 1);
			(*handle.create)(ptr(size++), value);
		}

		void *atRaw(nat id) {
			assert(id < size); // TODO: Throw a real exception here!
			return ptr(id);
		}

		// To string.
		virtual Str *STORM_FN toS();

		// Handle
		const Handle &handle;

		/**
		 * Base class for the iterator. This is not exposed to Storm, but is used internally to make
		 * the implementation easier.
		 * TODO: How does this fit with deepCopy?
		 */
		class Iter {
		public:
			// Pointing to the end.
			Iter();

			// Pointing to a specific element in 'owner'.
			Iter(Par<ArrayBase> owner, nat index = 0);

			// Compare.
			bool operator ==(const Iter &o) const;
			bool operator !=(const Iter &o) const;

			// Increase.
			Iter &operator ++();
			Iter operator ++(int);

		protected:
			// Array we're pointing to.
			Auto<ArrayBase> owner;

			// Index we're pointing to.
			nat index;

			// At end?
			bool atEnd() const;
		};

	protected:
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
		TYPE_EXTRA_CODE;
	public:
		static Type *stormType(Engine &e) { return arrayType(e, value<T>(e)); }

		// Empty array.
		Array() : ArrayBase(storm::handle<T>()) { setVTable(this); }

		// Copy array.
		Array(Par<Array<T>> o) : ArrayBase(o) { setVTable(this); }

		// Element access.
		T &at(Nat i) {
			assert(i < size);
			return ((T *)data)[i];
		}

		const T &at(Nat i) const {
			assert(i < size);
			return ((T *)data)[i];
		}

		// Get the last element.
		T &last() {
			assert(any());
			return ((T *)data)[size - 1];
		}

		const T &last() const {
			assert(any());
			return ((T *)data)[size - 1];
		}

		// Insert an element.
		void push(const T &item) {
			pushRaw(&item);
		}

		/**
		 * Iterator.
		 */
		class Iter : public ArrayBase::Iter {
		public:
			Iter() {}

			Iter(Par<Array<T>> owner, nat index = 0) : ArrayBase::Iter(owner, index) {}

			T &operator *() const {
				Array<T> *t = (Array<T> *)owner.borrow();
				return t->at(index);
			}

			T *operator ->() const {
				return &operator *();
			}
		};

		// Create iterators.
		Iter begin() {
			return Iter(this, nat(0));
		}

		Iter end() {
			return Iter(this, count());
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
		ArrayP(Par<ArrayP<T>> o) : Array(o) {}
	};

}
