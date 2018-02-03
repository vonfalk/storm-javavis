#include "stdafx.h"
#include "Sort.h"

namespace storm {

	/**
	 * SortData.
	 */

	static void init(SortData &d) {
		if (d.compare)
			d.compareFn = d.compare->rawCall();

		assert(d.data->filled < d.data->count, L"Sorting requires at least one free element.");
	}

	SortData::SortData(GcArray<byte> *data, const Handle &type) :
		data(data), type(type), compare(null), begin(0), end(data->filled) { init(*this); }

	SortData::SortData(GcArray<byte> *data, const Handle &type, FnBase *compare) :
		data(data), type(type), compare(compare), begin(0), end(data->filled) { init(*this); }

	SortData::SortData(GcArray<byte> *data, const Handle &type, size_t begin, size_t end) :
		data(data), type(type), compare(null), begin(begin), end(end) { init(*this); }

	SortData::SortData(GcArray<byte> *data, const Handle &type, FnBase *compare, size_t begin, size_t end) :
		data(data), type(type), compare(compare), begin(begin), end(end) { init(*this); }

	SortData::SortData(const SortData &src, size_t begin, size_t end) :
		data(src.data), type(src.type), compare(src.compare), compareFn(src.compareFn), begin(begin), end(end) {}

	/**
	 * Convenience operations.
	 */

	// Compare two elements.
	static inline bool compare(const SortData &d, const void *params[2]) {
		if (d.compare) {
			bool r = false;
			d.compareFn.call(d.compare, &r, params);
			return r;
		} else {
			return (*d.type.lessFn)(params[0], params[1]);
		}
	}

	static inline bool compare(const SortData &d, size_t a, size_t b) {
		size_t size = d.type.size;

		const void *params[2] = {
			d.data->v + a*size,
			d.data->v + b*size,
		};

		return compare(d, params);
	}

	static inline bool compare(const SortData &d, size_t a, const void *b) {
		size_t size = d.type.size;

		const void *params[2] = {
			d.data->v + a*size,
			b,
		};

		return compare(d, params);
	}

	// Move 'from' to 'to'. Assuming 'to' is not initialized.
	static inline void move(const SortData &d, size_t to, size_t from) {
		if (to == from)
			return;

		// Note: Just using memcpy is safe. The GC moves memory around all the time.
		size_t size = d.type.size;
		memcpy(d.data->v + to*size, d.data->v + from*size, size);
	}

	// Move 'from' to 'to' while keeping 'preserve' intact. Assumes 'from' is not to be preserved.
	static inline void move(const SortData &d, size_t &preserve, size_t to, size_t from) {
		if (preserve == to) {
			preserve = d.data->filled;
			move(d, preserve, to);
		}

		move(d, to, from);
	}


	/**
	 * Heap operations.
	 */

	static inline size_t left(const SortData &sort, size_t item) {
		return (item - sort.begin)*2 + 1 + sort.begin;
	}

	static inline size_t right(const SortData &sort, size_t item) {
		return (item - sort.begin)*2 + 2 + sort.begin;
	}

	static inline size_t parent(const SortData &sort, size_t item) {
		return (item - sort.begin - 1) / 2 + sort.begin;
	}

	// 'elem' is the location of the element that should be at location 'top'.
	static inline void siftDown(const SortData &sort, size_t top, size_t elem) {
		size_t at = top;
		while (at < sort.end) {
			size_t l = left(sort, at);
			size_t r = right(sort, at);

			// Done?
			if ((l >= sort.end || !compare(sort, elem, l))
				&& (r >= sort.end || !compare(sort, elem, r)))
				break;

			// Pick one and move.
			if (r < sort.end && compare(sort, l, r)) {
				move(sort, elem, at, r);
				at = r;
			} else {
				move(sort, elem, at, l);
				at = l;
			}
		}

		// Move the original element to its final position if needed.
		move(sort, at, elem);
	}

	void makeHeap(const SortData &sort) {
		size_t top = parent(sort, sort.end) + 1;
		while (top > sort.begin) {
			top--;

			// See if this element needs to move down the heap.
			siftDown(sort, top, top);
		}
	}

	void heapInsert(const void *elem, const SortData &sort) {
		// Sift up until we're done.
		size_t at = sort.end;
		while (at != sort.begin) {
			size_t p = parent(sort, at);

			// Done?
			if (!compare(sort, p, elem))
				break;

			// Move the parent one step down in the tree.
			move(sort, at, p);
			at = p;
		}

		// Insert the new element in the heap.
		sort.type.safeCopy(sort.data->v + at*sort.type.size, elem);
	}

	void heapRemove(const SortData &sort) {
		if (sort.begin + 1 >= sort.end)
			return;

		// Move the top element out of the way.
		move(sort, sort.data->filled, sort.begin);

		// Update the heap.
		siftDown(sort, sort.begin, sort.end - 1);

		// Move the extracted element to its proper location.
		move(sort, sort.end - 1, sort.data->filled);
	}

	void heapSort(const SortData &sort) {
		SortData heap = sort;

		makeHeap(heap);

		// Pop the elements one by one.
		while (heap.begin + 1 < heap.end) {
			heapRemove(heap);
			heap.end--;
		}
	}

	void insertionSort(const SortData &sort) {
		size_t scratch = sort.data->filled;

		for (size_t target = sort.begin + 1; target < sort.end; target++) {
			// Do we need to do anything at all with this element?
			if (!compare(sort, target, target - 1))
				continue;

			// Move it to our scratch space, move the element we already compared and keep comparing
			// until we find a suitable spot.
			move(sort, scratch, target);

			size_t to = target;
			do {
				move(sort, to, to - 1);
				to--;
			} while (to > sort.begin && compare(sort, scratch, to - 1));

			// Put the element we moved back where it belongs!
			move(sort, to, scratch);
		}
	}


	/**
	 * Quicksort.
	 */

	static inline size_t pickPivot(const SortData &now) {
		// Pick the median of three elements to get better performance in quicksort.
		size_t a = now.begin;
		size_t b = now.end - 1;
		size_t c = (b - a)/2 + a;

		// Small partition? Just pick one of them.
		if (a == c || b == c)
			return c;

		if (compare(now, a, b)) { // a < b
			if (compare(now, b, c))
				return b; // a < b < c
			else if (compare(now, c, a))
				return a; // c < a < b
			else
				return c; // a < c < b
		} else { // b < a
			if (compare(now, c, b))
				return b; // c < b < a
			else if (compare(now, a, c))
				return a; // b < a < c
			else
				return c; // b < c < a
		}
	}

	static inline size_t partition(const SortData &now, size_t pivot) {
		// Move the pivot away into temporary storage.
		size_t temp = now.data->filled;
		move(now, temp, pivot);

		// Make sure 'now.begin' is empty.
		if (pivot != now.begin)
			move(now, pivot, now.begin);

		// Partition.
		size_t l = now.begin, r = now.end - 1;
		while (l < r) {
			// Find an element from the right to put in the hole to the left.
			while (l < r && !compare(now, r, temp))
				r--;
			if (l < r)
				move(now, l++, r);

			// Find an element from the left to put in the hole to the right.
			while (l < r && !compare(now, temp, l))
				l++;
			if (l < r)
				move(now, r--, l);
		}

		// Move the pivot back into its proper location and return its location.
		move(now, l, temp);
		return l;
	}

	void sort(const SortData &sort) {
		SortData now = sort;

		// Stack of remembered ranges.
		const size_t STACK_SIZE = 30;
		size_t begin[STACK_SIZE];
		size_t end[STACK_SIZE];
		size_t top = 0;

		// Minimum number of elements actually handled by quicksort.
		const size_t INSERTION_LIMIT = 16;

		while (true) {
			if (now.begin + 1 >= now.end) {
				// Try to pop some work from the stack!
				if (top > 0) {
					top--;
					now.begin = begin[top];
					now.end = end[top];
				} else {
					// I guess we're done.
					break;
				}
			}

			// Small enough to fall back to insertion sort?
			if (now.end - now.begin <= INSERTION_LIMIT) {
				insertionSort(now);
				now.begin = now.end;
				continue;
			}

			// Out of stack space? Fall back to heap sort.
			if (top >= STACK_SIZE) {
				heapSort(now);
				now.begin = now.end;
				continue;
			}

			// Pick a pivot and use that.
			size_t pivot = pickPivot(now);
			pivot = partition(now, pivot);

			// Put the smaller part on hold for later. Take the larger one now.
			begin[top] = now.begin;
			end[top] = now.end;

			if (pivot - now.begin < now.end - pivot) {
				end[top] = pivot;
				now.begin = pivot + 1;
			} else {
				begin[top] = pivot + 1;
				now.end = pivot;
			}

			// Only 'commit' the transaction if it was a nonzero number of elements.
			if (begin[top] + 1 < end[top])
				top++;
		}
	}

}
