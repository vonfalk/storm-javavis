#include "stdafx.h"
#include "Core/Sort.h"

std::wostream &operator <<(std::wostream &to, const GcArray<Int> *src) {
	to << L"[";

	if (src->filled)
		to << src->v[0];

	for (size_t i = 1; i < src->filled; i++)
		to << L", " << src->v[i];

	return to << "]";
}

static bool sorted(const GcArray<Int> *src) {
	for (size_t i = 1; i < src->filled; i++)
		if (src->v[i - 1] > src->v[i])
			return false;

	return true;
}

static bool sorted(const GcArray<Int> *src, size_t from, size_t to) {
	for (size_t i = from + 1; i < to; i++)
		if (src->v[i - 1] > src->v[i])
			return false;

	return true;
}

static SortData data(GcArray<Int> *array, const Handle &h, size_t from, size_t to) {
	return SortData((GcArray<byte> *)array, h, from, to);
}

// Try out the low-level sorting functions in ways that the regular interfaces do not test very often.
BEGIN_TEST(Sort, Core) {
	Engine &e = gEngine();

	const Handle &h = StormInfo<Int>::handle(e);
	GcArray<Int> *array = runtime::allocArray<Int>(e, h.gcArrayType, 21);
	array->filled = array->count - 1;
	for (size_t i = 0; i < array->filled; i++)
		array->v[i] = Int(array->filled - i);

	CHECK(!sorted(array));

	// Sort the last few elements with heapsort.
	heapSort(data(array, h, 10, 20));
	CHECK(sorted(array, 10, 20));

	// Sort the other range using insertion sort.
	insertionSort(data(array, h, 8, 16));
	CHECK(sorted(array, 8, 16));

	// Sort with quicksort.
	sort(data(array, h, 8, 20));
	CHECK(sorted(array, 8, 20));

} END_TEST
