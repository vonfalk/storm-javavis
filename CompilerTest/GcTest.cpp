#include "stdafx.h"
#include "Test/Test.h"
#include "Compiler/Debug.h"

using namespace storm::debug;

// Create a list of links, containing elements 0 to n-1
static Link *createList(nat n) {
	Link *start = null;
	Link *prev = null;

	for (nat i = 0; i < n; i++) {
		Link *now = new (*gEngine) Link;
		now->value = i;

		if (prev == null) {
			start = now;
		} else {
			prev->next = now;
		}
		prev = now;
	}

	return start;
}

// Check a list.
static bool checkList(Link *first, nat n) {
	Link *at = first;
	for (nat i = 0; i < n; i++) {
		if (!at)
			return false;
		if (at->value != i)
			return false;

		at = at->next;
	}

	return at == null;
}

BEGIN_TEST(GcTest1, GcScan) {
	Engine &e = *gEngine;

	// Allocate this many nodes to make sure MPS will try to GC at least once!
	const nat count = 100000;

	Link *start = createList(count);
	CHECK(checkList(start, count));

} END_TEST

/**
 * Long-running stresstest of the GC logic. Too slow for regular use, but good when debugging.
 */
BEGIN_TEST(GcTest2, Stress) {
	Engine &e = *gEngine;

	// Allocate this many nodes.
	const nat count = 1000;

	for (nat z = 0; z < 10; z++) {
		ValClass *start = null;
		ValClass *prev = null;

		for (nat i = 0; i < count; i++) {
			ValClass *now = new (*gEngine) ValClass;
			now->data.value = i;
			now->data.list = createList(i + 1);

			if (prev) {
				prev->next = now;
			} else {
				start = now;
			}
			prev = now;
		}

		ValClass *at = start;
		for (nat i = 0; i < count; i++) {
			if (at->data.value != i) {
				CHECK_EQ(at->data.value, i);
				break;
			}
			if (!checkList(at->data.list, i + 1)) {
				CHECK(checkList(at->data.list, i + 1));
				break;
			}

			at = at->next;
		}

		CHECK_EQ(at, (ValClass *)null);
	}

} END_TEST
