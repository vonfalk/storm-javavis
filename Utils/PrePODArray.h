#pragma once

/**
 * Array with a preallocated part and a dynamic part. This is
 * to accommodate cases where there is a common case where the
 * number of elements required is small, and a rare case where
 * the number of elements is larger. Using this array, the common
 * case can use automatic storage (eg. the stack), while we can
 * still gracefully handle the rare cases by dynamically allocate
 * memory.
 * This version is intended for POD:s, ie types where it is fine
 * to not call constructors and destructors. For the generic case,
 * please use PreArray in PreArray.h.
 */
template <class T, nat n>
class PrePODArray {
public:
	// Create.
	PrePODArray() : dyn(null), size(0), capacity(n) {}

	// Copy.
	PrePODArray(const PrePODArray &o) : dyn(null), size(0), capacity(n) {
		ensure(o.size());
		size = o.size;
		copyArray(pre, o.pre, size);
		if (size > n)
			copyArray(dyn, o.dyn, size - n);
	}

	// Assign.
	PrePODArray<T, n> &operator =(const PrePODArray<T, n> &o) {
		clear();
		ensure(o.size);
		for (nat i = 0; i < o.size; i++)
			push(o[i]);
		return *this;
	}

	// Destroy.
	~PrePODArray() {
		clear();
	}

	// Clear.
	void clear() {
		delete []dyn;
		dyn = null;
		capacity = n;
		size = 0;
	}

	// Element access.
	T &operator [](nat id) {
		return *ptr(id);
	}

	const T &operator [](nat id) const {
		return *ptr(id);
	}

	// Push new elements.
	void push(const T &v) {
		ensure(size + 1);
		*ptr(size++) = v;
	}

	// # of elements
	inline nat count() const {
		return size;
	}

private:
	// Preallocated storage.
	T pre[n];

	// Dynamic storage.
	T *dyn;

	// Used until.
	nat size;

	// Allocated size (all storage).
	nat capacity;

	// Ensure capacity.
	void ensure(nat elems) {
		if (elems <= capacity)
			return;

		capacity = max(elems, capacity * 2);
		T *z = new T[capacity - n];
		if (dyn != null && size > n)
			copyArray(z, dyn, size - n);
		swap(dyn, z);
		delete []z;
	}

	// Get a pointer to an element.
	inline T *ptr(nat id) const {
		if (id >= n)
			return (T *)&dyn[id - n];
		else
			return (T *)&pre[id];
	}
};
