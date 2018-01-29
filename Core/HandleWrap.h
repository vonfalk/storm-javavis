#pragma once
#include "Handle.h"

namespace storm {
	using std::ptrdiff_t;

	/**
	 * Wrap a GcArray in iterators that can be manipulated by algorithms in the C++ standard library.
	 *
	 * TODO: We need to make sure 'HandleValue' instances stored on the heap works properly.
	 */

	class HandleRef;


	/**
	 * Value returned by dereferencing the iterator.
	 */
	class HandleValue {
		friend class HandleRef;
	public:
		// Copy.
		HandleValue(const HandleValue &src);
		HandleValue(const HandleRef &src);
		HandleValue &operator =(const HandleValue &o);
		HandleValue &operator =(const HandleRef &o);

		// Destroy.
		~HandleValue();

		// Compare using whatever is in the handle.
		bool operator <(const HandleValue &o) const;
		bool operator <(const HandleRef &o) const;
		bool operator ==(const HandleValue &o) const;
		bool operator ==(const HandleRef &o) const;

	private:
		// Handle.
		const Handle &type;

		// Data.
		GcArray<byte> *data;
	};


	/**
	 * Reference to a value inside an iterator.
	 */
	class HandleRef {
		friend class HandleValue;
	public:
		// Create.
		HandleRef(const Handle &type, byte *data);
		HandleRef(const HandleValue &src);

		// Assign from a Value or another reference.
		HandleRef &operator =(const HandleValue &val);
		HandleRef &operator =(const HandleRef &ref);

		// Compare using whatever is in the handle.
		bool operator <(const HandleRef &o) const;
		bool operator <(const HandleValue &o) const;
		bool operator ==(const HandleRef &o) const;
		bool operator ==(const HandleValue &o) const;

	private:
		// Handle.
		const Handle &type;

		// Data.
		byte *data;
	};


	/**
	 * Iterator inside a GcArray containing values described in terms of a Handle. Not possible to
	 * store in a heap-allocated object since these contain pointers into the GcArray.
	 */
	class HandleIter {
	public:
		// Tell the STL what we're capable of.
		typedef std::random_access_iterator_tag iterator_category;
		typedef HandleValue value_type;
		typedef ptrdiff_t difference_type;
		typedef HandleValue *pointer;
		typedef HandleRef reference; // HandleValue classes act like references.

		// Construct from an element inside a GcArray.
		HandleIter(const Handle &type, GcArray<byte> *data, size_t element);

		// Walk through the list.
		HandleIter &operator ++();
		HandleIter operator ++(int);
		HandleIter &operator --();
		HandleIter operator --(int);

		// Jump forward/backward.
		HandleIter &operator +=(ptrdiff_t o);
		HandleIter &operator -=(ptrdiff_t o);

		// Compute size difference.
		ptrdiff_t operator -(const HandleIter &o) const;

		// Compare.
		inline bool operator <(const HandleIter &o) const { return data < o.data; }
		inline bool operator >(const HandleIter &o) const { return data > o.data; }
		inline bool operator <=(const HandleIter &o) const { return data <= o.data; }
		inline bool operator >=(const HandleIter &o) const { return data >= o.data; }
		inline bool operator ==(const HandleIter &o) const { return data == o.data; }
		inline bool operator !=(const HandleIter &o) const { return data != o.data; }

		// Extract elements.
		inline HandleRef operator *() const { return HandleRef(*type, data); }
		inline HandleRef operator [](ptrdiff_t id) const { return HandleRef(*type, data + type->size*id); }

	private:
		// Handle.
		const Handle *type;

		// Pointer to the data.
		byte *data;
	};

	// Non-member operators.
	inline HandleIter operator +(const HandleIter &i, ptrdiff_t o) {
		HandleIter t = i;
		t += o;
		return t;
	}
	inline HandleIter operator +(ptrdiff_t o, const HandleIter &i) {
		HandleIter t = i;
		t += o;
		return t;
	}
	inline HandleIter operator -(const HandleIter &i, ptrdiff_t o) {
		HandleIter t = i;
		t -= o;
		return t;
	}

}
