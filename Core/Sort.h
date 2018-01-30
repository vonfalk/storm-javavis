#pragma once
#include "Handle.h"

namespace storm {

	/**
	 * Utilities for sorting elements in a GcArray of unknown type. Note: these functions require
	 * that 'data' contains an unused element that can be used for temporary storage.
	 */

	/**
	 * Description of a range to operate on. Any temporary data will be placed at the location
	 * indicated by 'data->filled'.
	 */
	class SortData {
	public:
		// Use the entire available range in 'data'.
		SortData(GcArray<byte> *data, const Handle &type);
		SortData(GcArray<byte> *data, const Handle &type, Handle::LessFn compare);

		// Use only a part of 'data'.
		SortData(GcArray<byte> *data, const Handle &type, size_t begin, size_t end);
		SortData(GcArray<byte> *data, const Handle &type, Handle::LessFn compare, size_t begin, size_t end);

		// Narrow the described region.
		SortData(const SortData &src, size_t begin, size_t end);

		// Array to operate on.
		GcArray<byte> *data;

		// Type of data to use.
		const Handle &type;

		// Comparison function.
		Handle::LessFn compare;

		// Range to be affected by the current operation.
		size_t begin;
		size_t end;
	};

	// Make a max-heap out of the elements in 'data'. Runs in O(n) time.
	void makeHeap(const SortData &data);

	// Insert an element into a max-heap.
	void heapInsert(void *elem, const SortData &data);

	// Remove an element from a max-heap. The removed element is moved to the last position of the heap.
	void heapRemove(const SortData &data);

	// Heap sort using the max-heap.
	void heapSort(const SortData &data);

	// Sort. Using quicksort but falls back to heapsort if necessary.
	void sort(const SortData &data);

}
