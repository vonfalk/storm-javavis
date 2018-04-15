#pragma once

namespace os {

	/**
	 * A table that contains pointer to T, and allows iterating through them in some order that is
	 * consistent until elements are added and removed.
	 *
	 * Automatically resizes the underlying storage as needed (both growing and shrinking).
	 */
	template <class T>
	class Table {
	public:
		// Create.
		Table() : capacity(0), filled(0), hint(0), data(null) {}

		// Destroy.
		~Table() {
			delete []data;
		}

		// Add an element and return its current key.
		size_t put(T *elem) {
			if (filled == capacity)
				grow();

			while (data[hint]) {
				if (++hint == capacity)
					hint = 0;
			}

			filled++;
			data[hint] = elem;
			return hint;
		}

		// Remove the element 'id' and return it.
		T *remove(size_t id) {
			if (id >= capacity)
				return null;

			T *r = data[id];
			data[id] = null;
			if (r) {
				--filled;
				shrink();
			}

			return r;
		}

		// Get the element at index 'id'. May be null.
		T *operator [] (size_t id) const {
			return data[id];
		}

		// Total number of elements.
		size_t size() const {
			return capacity;
		}

		// Number of elements in here.
		size_t used() const {
			return filled;
		}

		// Find the first element's index and return it. Returns >= size() if no elements exist.
		size_t first() const {
			for (size_t i = 0; i < capacity; i++)
				if (data[i])
					return i;

			return capacity;
		}

		// Find the next element's index and return it. Returns something >= size() if no more elements exist.
		size_t next(size_t curr) const {
			for (size_t i = curr + 1; i < capacity; i++)
				if (data[i])
					return i;

			return capacity;
		}

	private:
		Table(const Table &);
		Table &operator =(const Table &);

		// Total capacity.
		size_t capacity;

		// Number of used entries.
		size_t filled;

		// Last filled element. The next one is probably empty.
		size_t hint;

		// The actual data.
		T **data;

		// Grow to fit at least one more element.
		void grow() {
			size_t cap = max(capacity * 2, size_t(8));
			T **d = new T*[cap];

			hint = 0;
			for (size_t i = first(); i < capacity; i = next(i))
				d[hint++] == data[i];

			delete []data;
			data = d;
			capacity = cap;
		}

		// Shrink if necessary.
		void shrink() {
			size_t cap = capacity / 4;
			// Too many elements?
			if (filled > cap)
				return;

			T **d = new T*[cap];

			hint = 0;
			for (size_t i = first(); i < capacity; i = next(i))
				d[hint++] = data[i];

			delete []data;
			data = d;
			capacity = cap;
		}
	};

}
