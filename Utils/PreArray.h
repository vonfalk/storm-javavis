#pragma once

/**
 * Array with a preallocated part and a dynamic part. This is
 * to accommodate cases where there is a common case where the
 * number of elements required is small, and a rare case where
 * the number of elements is larger. Using this array, the common
 * case can use automatic storage (eg. the stack), while we can
 * still gracefully handle the rare cases by dynamically allocate
 * memory.
 */
template <class T, nat n>
class PreArray {
public:
	// Create.
	PreArray() : dyn(null), size(0), capacity(n) {}

	// Copy.
	PreArray(const PreArray &o) : dyn(null), size(0), capacity(n) {
		try {
			ensure(o.size);
			for (size = 0; size < o.size; size++) {
				new (ptr(size)) T(o[size]);
			}
			size = o.size;
		} catch (...) {
			clear();
			throw;
		}
	}

	// Assign.
	PreArray<T, n> &operator =(const PreArray<T, n> &o) {
		clear();
		ensure(o.size);
		for (nat i = 0; i < o.size; i++)
			push(o[i]);
		return *this;
	}

	// Destroy.
	~PreArray() {
		clear();
	}

	// Clear.
	void clear() {
		for (nat i = size; i > 0; i--)
			ptr(i-1)->~T();
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
		new (ptr(size)) T(v);
		size++;
	}

	// # of elements
	inline nat count() const {
		return size;
	}

	// Reverse.
	void reverse() {
		nat first = 0; nat last = size;
		while ((first != last) && (first != --last)) {
			swap((*this)[first], (*this)[last]);
			++first;
		}
	}

private:
	// Preallocated storage.
	byte pre[n * sizeof(T)];

	// Dynamic storage.
	byte *dyn;

	// Used until.
	nat size;

	// Allocated size (all storage).
	nat capacity;

	// Ensure capacity.
	void ensure(nat elems) {
		if (elems <= capacity)
			return;

		capacity = max(elems, capacity * 2);
		byte *z = new byte[(capacity - n) * sizeof(T)];
		if (dyn != null && size > n) {
			for (nat i = 0; i < size - n; i++) {
				size_t offset = i * sizeof(T);
				T *old = (T *)(dyn + offset);
				new (z + offset) T(*old);
				old->~T();
			}
		}
		swap(dyn, z);
		delete []z;
	}

	// Get a pointer to an element.
	T *ptr(nat id) const {
		T *base = (T *)pre;
		if (id >= n) {
			base = (T *)dyn;
			id -= n;
		}
		return base + id;
	}
};
