#include "stdafx.h"
#include "Sort.h"

namespace storm {

	/**
	 * SortData.
	 */

	static void check(const SortData &d) {
		assert(d.data->filled < d.data->count, L"Sorting requires at least one free element.");
	}

	SortData::SortData(GcArray<byte> *data, const Handle &type) :
		data(data), type(type), compare(type.lessFn), begin(0), end(data->filled) { check(*this); }

	SortData::SortData(GcArray<byte> *data, const Handle &type, Handle::LessFn compare) :
		data(data), type(type), compare(compare), begin(0), end(data->filled) { check(*this); }

	SortData::SortData(GcArray<byte> *data, const Handle &type, size_t begin, size_t end) :
		data(data), type(type), compare(type.lessFn), begin(begin), end(end) { check(*this); }

	SortData::SortData(GcArray<byte> *data, const Handle &type, Handle::LessFn compare, size_t begin, size_t end) :
		data(data), type(type), compare(compare), begin(begin), end(end) { check(*this); }

	SortData::SortData(const SortData &src, size_t begin, size_t end) :
		data(src.data), type(src.type), compare(src.compare), begin(begin), end(end) {}

	/**
	 * Convenience operations.
	 */

	// Compare two elements.
	static inline bool compare(const SortData &d, size_t a, size_t b) {
		size_t size = d.type.size;
		return (*d.compare)(d.data->v + a*size, d.data->v + b*size);
	}

	// Move 'from' to 'to'. Assuming 'to' is not initialized.
	static inline void move(const SortData &d, size_t to, size_t from) {
		if (to == from)
			return;

		size_t size = d.type.size;
		d.type.safeCopy(d.data->v + to*size, d.data->v + from*size);
		d.type.safeDestroy(d.data->v + from*size);
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

	static inline size_t left(size_t item) {
		return item*2 + 1;
	}

	static inline size_t right(size_t item) {
		return item*2 + 2;
	}

	static inline size_t parent(size_t item) {
		return (item - 1) / 2;
	}

	// 'elem' is the location of the element that should be at location 'top'
	static inline void siftDown(const SortData &sort, size_t top, size_t elem) {
		size_t at = top;
		while (at < sort.end) {
			size_t l = left(at);
			size_t r = right(at);

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
		size_t top = parent(sort.end) + 1;
		while (top > 0) {
			top--;

			// See if this element needs to move down the heap.
			siftDown(sort, top, top);
		}
	}

	void heapInsert(void *elem, const SortData &sort) {
		assert(false, L"Not implemented yet!");
	}

	void heapRemove(const SortData &sort) {
		if (sort.begin + 1 >= sort.end)
			return;

		// Move the top element out of the way.
		move(sort, sort.data->filled, 0);

		// Update the heap.
		siftDown(sort, 0, sort.end - 1);

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

			// TODO: Pick better pivot!
			size_t pivot = pickPivot(now);
			pivot = partition(now, pivot);

			// Put the smaller part on hold for later. Take the larger one now.
			size_t qBegin = now.begin;
			size_t qEnd   = now.end;
			if (pivot - now.begin < now.end - pivot) {
				qEnd = pivot;
				now.begin = pivot + 1;
			} else {
				qBegin = pivot + 1;
				now.end = pivot;
			}

			// Put it on the stack if there is room. Otherwise, fall back to heapsort.
			if (qBegin + 1 < qEnd) {
				if (top < STACK_SIZE) {
					begin[top] = qBegin;
					end[top] = qEnd;
					top++;
				} else {
					std::swap(qBegin, now.begin);
					std::swap(qEnd, now.end);

					heapSort(now);

					std::swap(qBegin, now.begin);
					std::swap(qEnd, now.end);
				}
			}
		}
	}

}
