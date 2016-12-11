#pragma once
#include "Object.h"
#include "Handle.h"
#include "GcArray.h"
#include "Utils/Exception.h"

namespace storm {
	STORM_PKG(core);

	/**
	 * Custom error type.
	 */
	class ArrayError : public Exception {
	public:
		ArrayError(const String &msg) : msg(msg) {}
		virtual String what() const { return L"Array error: " + msg; }
	private:
		String msg;
	};

	/**
	 * Base class for arrays. Implements a slightly inconvenient interface that is reusable
	 * regardless of the contained type. Use the derived class Array<> for C++.
	 */
	class ArrayBase : public Object {
		STORM_CLASS;
	public:
		// Empty array.
		ArrayBase(const Handle &type);

		// Create an array with 'n' copies of 'data'.
		ArrayBase(const Handle &type, Nat n, const void *data);

		// Copy another array.
		STORM_CTOR ArrayBase(ArrayBase *other);

		// Deep copy.
		virtual void STORM_FN deepCopy(CloneEnv *env);

		// Get size.
		inline Nat STORM_FN count() const { return data ? data->filled : 0; }

		// Reserve size.
		void STORM_FN reserve(Nat count);

		// Clear.
		void STORM_FN clear();

		// Any elements?
		inline Bool STORM_FN any() const { return count() > 0; }

		// Empty?
		inline Bool STORM_FN empty() const { return count() == 0; }

		// Erase an element.
		void STORM_FN erase(Nat id);

		// Insert an element, giving it the id 'id'. 'id' <= 'count()'.
		void CODECALL insertRaw(Nat id, const void *item);

		// Reverse the array.
		void STORM_FN reverse();

		// To string.
		virtual void STORM_FN toS(StrBuf *to) const;

		// Handle of the contained type.
		const Handle &handle;

		// First element access.
		inline void *CODECALL firstRaw() const {
			return getRaw(0);
		}

		// Last element access.
		inline void *CODECALL lastRaw() const {
			return getRaw(count() - 1);
		}

		// Raw element access.
		inline void *CODECALL getRaw(Nat id) const {
			if (id > count())
				throw ArrayError(L"Index " + ::toS(id) + L" out of bounds (of " + ::toS(count()) + L").");
			return ptr(id);
		}

		// Push an element.
		void CODECALL pushRaw(const void *element);


		/**
		 * Base class for the iterator. Not exposed to Storm, but it is used internally to make the
		 * implementation of the Storm iterator easier.
		 * TODO: How does this fit with deepCopy?
		 */
		class Iter {
		public:
			// Pointing to the end.
			Iter();

			// Pointing to a specific element in 'owner'.
			Iter(ArrayBase *owner, Nat index = 0);

			// Compare.
			bool operator ==(const Iter &o) const;
			bool operator !=(const Iter &o) const;

			// Increase.
			Iter &operator ++();
			Iter operator ++(int);

			// Raw get function.
			void *CODECALL getRaw() const;

			// Get the current index.
			inline Nat getIndex() const { return index; }

			// Raw pre- and post increment.
			Iter CODECALL preIncRaw();
			Iter CODECALL postIncRaw();

		private:
			// Array we're pointing to.
			ArrayBase *owner;

			// Index.
			Nat index;

			// At end?
			bool atEnd() const;
		};

		// Raw iterator access.
		Iter CODECALL beginRaw();
		Iter CODECALL endRaw();

	protected:
		// Array contents (of bytes, to make addressing easier).
		GcArray<byte> *data;

		// Get the pointer to an element.
		inline void *ptr(GcArray<byte> *data, Nat id) const { return data->v + (id * handle.size); }
		inline void *ptr(Nat id) const { return ptr(data, id); }

		// Ensure 'data' can hold at least 'n' objects.
		void ensure(Nat n);
	};

	// Declare the array's template in Storm.
	STORM_TEMPLATE(Array, createArray);

	/**
	 * Class used from C++.
	 *
	 * TODO: Set vtables for class in constructors.
	 */
	template <class T>
	class Array : public ArrayBase {
		STORM_SPECIAL;
	public:
		// Get the Storm type for this object.
		static Type *stormType(Engine &e) {
			return runtime::cppTemplate(e, ArrayId, 1, StormInfo<T>::id());
		}

		// Empty array.
		Array() : ArrayBase(StormInfo<T>::handle(engine())) {
			runtime::setVTable(this);
		}

		// 'n' elements.
		Array(Nat n, const T &item = T()) : ArrayBase(StormInfo<T>::handle(engine()), n, &item) {
			runtime::setVTable(this);
		}

		// Copy array.
		Array(Array<T> *o) : ArrayBase(o) {
			runtime::setVTable(this);
		}

		// Create an array with one element in it.
		Array(const T &item) : ArrayBase(StormInfo<T>::handle(engine())) {
			runtime::setVTable(this);
			push(item);
		}

		// Element access.
		T &at(Nat i) {
			return *(T *)getRaw(i);
		}

		const T &at(Nat i) const {
			return *(const T *)getRaw(i);
		}

		// Get the last element (if any).
		T &last() {
			return *(T *)lastRaw();
		}

		const T &first() const {
			return *(const T *)firstRaw();
		}

		const T &last() const {
			return *(const T *)lastRaw();
		}

		// Insert an element.
		void push(const T &item) {
			pushRaw(&item);
		}

		// Insert at a specific location.
		void insert(Nat pos, const T &item) {
			insertRaw(pos, &item);
		}

		Array<T> &operator <<(const T &item) {
			push(item);
			return *this;
		}

		/**
		 * Iterator.
		 */
		class Iter : public ArrayBase::Iter {
		public:
			Iter() : ArrayBase::Iter() {}

			Iter(Array<T> *owner, Nat index = 0) : ArrayBase::Iter(owner, index) {}

			T &operator *() const {
				return *(T *)getRaw();
			}

			T *operator ->() const {
				return (T *)getRaw();
			}
		};

		// Create iterators.
		Iter begin() {
			return Iter(this, 0);
		}

		Iter end() {
			return Iter(this, count());
		}
	};


	/**
	 * C++ helper for joining an array or similar into a string.
	 */
	template <class T>
	void join(StrBuf *to, Array<T> *src, const wchar *space) {
		if (src->count() == 0)
			return;

		*to << src->at(0);

		for (Nat i = 1; i < src->count(); i++) {
			*to << space << src->at(i);
		}
	}

}
