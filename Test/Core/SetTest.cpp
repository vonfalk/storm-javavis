#include "stdafx.h"
#include "Core/Set.h"
#include "Core/Str.h"
#include "Core/GcBitset.h"
#include "Compiler/Debug.h"

using debug::PtrKey;

BEGIN_TEST(SetTest, Core) {
	Engine &e = gEngine();

	// Basic operation:
	{
		Set<Str *> *set = new (e) Set<Str *>();

		set->put(new (e) Str(L"A"));
		set->put(new (e) Str(L"B"));
		set->put(new (e) Str(L"A"));
		set->put(new (e) Str(L"E"));

		CHECK_EQ(set->count(), 3);
		CHECK(set->has(new (e) Str(L"A")));

		set->remove(new (e) Str(L"A"));

		CHECK_EQ(set->count(), 2);
		CHECK(set->has(new (e) Str(L"B")));
		CHECK(set->has(new (e) Str(L"E")));
		CHECK(!set->has(new (e) Str(L"A")));
	}

	// TODO: More tests here!

} END_TEST


static bool moveObjects(Array<PtrKey *> *k) {
	// Try a few times...
	for (nat i = 0; i < 10; i++) {
		gEngine().gc.collect();

		for (nat i = 0; i < k->count(); i++)
			if (k->at(i)->moved())
				return true;
	}

	return false;
}

BEGIN_TEST(SetTestMove, Core) {
	// Do we handle moving objects properly?
	Engine &e = gEngine();

	const nat count = 10;

	// Store these in arrays so the GC can move them properly.
	Array<PtrKey *> *k = new (e) Array<PtrKey *>();

	for (nat i = 0; i < count; i++) {
		k->push(new (e) PtrKey());

		// Make it less likely that we pin all objects.
		runtime::allocArray<Byte>(e, &byteArrayType, 1000);
	}

	Set<PtrKey *> *set = new (e) Set<PtrKey *>();

	for (nat i = 0; i < count; i++) {
		set->put(k->at(i));
		k->at(i)->reset();
	}

	// Try to force objects to be moved around!
	// Note: if we're using a non-moving collector, the following tests will fail.
	CHECK(moveObjects(k));

	// Try to find all objects in the set:
	for (nat i = 0; i < count; i++) {
		CHECK_EQ(set->get(k->at(i)), k->at(i));
	}

	// Move the objects once more, so we can try 'remove' properly.
	CHECK(moveObjects(k));

	// Try to remove an object.
	for (nat i = 0; i < count; i++) {
		set->remove(k->at(i));
		CHECK_EQ(set->count(), count - i - 1);

		// Validate the others.
		for (nat j = 0; j < count; j++) {
			if (j <= i) {
				CHECK(!set->has(k->at(j)));
			} else {
				CHECK(set->has(k->at(j)));
			}
		}
	}

} END_TEST

BEGIN_TEST(GcBitsetTest, Core) {
	Engine &e = gEngine();

	GcBitset *s = allocBitset(e, 10);
	CHECK_EQ(s->count(), 10);

	for (nat i = 0; i < s->count(); i++)
		CHECK(!s->has(i));

	s->set(5, true);
	CHECK(s->has(5));
	CHECK(!s->has(4));
	CHECK(!s->has(6));

} END_TEST
