#include "stdafx.h"
#include "Compiler/Debug.h"
#include "Utils/Bitwise.h"

using namespace storm::debug;

BEGIN_TEST(GcTest1, GcScan) {
	Gc &g = gc();

	CHECK(g.test());
} END_TEST

// Create a list of links, containing elements 0 to n-1
static Link *createList(nat n) {
	Link *start = null;
	Link *prev = null;
	Engine &e = gEngine();

	for (nat i = 0; i < n; i++) {
		Link *now = new (e) Link;
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

BEGIN_TEST(GcTest2, GcObjects) {
	Engine &e = gEngine();

	// Allocate this many nodes to make sure MPS will try to GC at least once!
	const nat count = 100000;

	Link *start = createList(count);
	CHECK(checkList(start, count));

} END_TEST

/**
 * Long-running stresstest of the GC logic. Too slow for regular use, but good when debugging.
 */
BEGIN_TEST(GcTest3, Stress) {
	Engine &e = gEngine();

	// Allocate this many nodes.
	const nat count = 1000;

	for (nat z = 0; z < 10; z++) {
		ValClass *start = null;
		ValClass *prev = null;

		for (nat i = 0; i < count; i++) {
			ValClass *now = new (e) ValClass;
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

BEGIN_TEST(CodeAllocTest, GcObjects) {
	Engine &e = gEngine();

	nat count = 10;
	nat allocSize = sizeof(void *) * count + 3;

	void *code = runtime::allocCode(e, allocSize, count);

	GcCode *c = runtime::codeRefs(code);

	Array<PtrKey *> *o = new (e) Array<PtrKey *>();
	PtrKey **objs = (PtrKey **)code;

	for (nat i = 0; i < count; i++) {
		c->refs[i].offset = sizeof(void *)*i;
		c->refs[i].kind = GcCodeRef::rawPtr;

		PtrKey *c = new (e) PtrKey();
		objs[i] = c;
		o->push(c);
	}

	bool moved = false;
	while (!moved) {
		for (nat i = 0; i < o->count(); i++) {
			moved |= o->at(i)->moved();
		}

		createList(100);
		e.gc.collect();
	}

	CHECK_EQ(runtime::codeSize(code), roundUp(size_t(allocSize), sizeof(size_t)));
	CHECK_EQ(runtime::codeRefs(code)->refCount, 10);

	for (nat i = 0; i < count; i++) {
		CHECK_EQ(o->at(i), objs[i]);
	}

} END_TEST

BEGIN_TEST(CodeRelPtr, GcObjects) {
	Engine &e = gEngine();

	static GcType type = {
		GcType::tArray,
		null,
		null,
		sizeof(void *),
		1, { 0 },
	};

	nat count = 20;
	GcArray<void **> *codeArray = runtime::allocArray<void **>(e, &type, count);

	for (nat i = 0; i < count; i++) {
		void **code = (void **)runtime::allocCode(e, sizeof(void *) * 2, 2);
		GcCode *meta = runtime::codeRefs(code);
		meta->refs[0].offset = 0;
		meta->refs[0].kind = GcCodeRef::inside;
		*code = code + 1;

		codeArray->v[i] = code;
	}

	// Make stuff move!
	createList(1000);
	e.gc.collect();

	// Verify stuff!
	for (nat i = 0; i < count; i++) {
		void **code = codeArray->v[i];

		// We shall still point into the object.
		CHECK_EQ(*code, code + 1);
	}

} END_TEST;
