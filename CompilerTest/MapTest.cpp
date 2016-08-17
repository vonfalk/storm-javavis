#include "stdafx.h"
#include "Test/Test.h"
#include "Core/Map.h"
#include "Core/Str.h"
#include "Compiler/Debug.h"

using debug::PtrKey;


BEGIN_TEST(MapTest, Runtime) {
	Engine &e = *gEngine;

	// Basic operation:
	{
		Map<Str *, Str *> *map = new (e) Map<Str *, Str *>();

		map->put(new (e) Str(L"A"), new (e) Str(L"10"));
		map->put(new (e) Str(L"B"), new (e) Str(L"11"));
		map->put(new (e) Str(L"A"), new (e) Str(L"12"));
		map->put(new (e) Str(L"E"), new (e) Str(L"13"));

		CHECK_EQ(map->count(), 3);
		CHECK_EQ(::toS(map->get(new (e) Str(L"A"))), L"12");

		map->remove(new (e) Str(L"A"));

		CHECK_EQ(map->count(), 2);
		CHECK_EQ(::toS(map->get(new (e) Str(L"B"))), L"11");
		CHECK_EQ(::toS(map->get(new (e) Str(L"E"))), L"13");
		CHECK_EQ(::toS(map->get(new (e) Str(L"A"), new (e) Str(L"-"))), L"-")
	}

	// TODO: More tests here!

} END_TEST


bool moveObjects(Array<PtrKey *> *k) {
	// Try a few times...
	for (nat i = 0; i < 10; i++) {
		gEngine->gc.collect();

		for (nat i = 0; i < k->count(); i++)
			if (k->at(i)->moved())
				return true;
	}

	return false;
}

BEGIN_TEST(MapMoveTest, Runtime) {
	// Do we handle moving objects properly?
	Engine &e = *gEngine;

	const nat count = 10;

	// Store these in arrays so the GC can move them properly.
	Array<PtrKey *> *k = new (e) Array<PtrKey *>();
	Array<Str *> *v = new (e) Array<Str *>();

	for (nat i = 0; i < count; i++) {
		k->push(new (e) PtrKey());
		v->push(new (e) Str(::toS(i).c_str()));
	}

	Map<PtrKey *, Str *> *map = new (e) Map<PtrKey *, Str *>();

	for (nat i = 0; i < count; i++) {
		map->put(k->at(i), v->at(i));
		k->at(i)->reset();
	}

	// Try to force objects to be moved around!
	// Note: if we're using a non-moving collector, the following tests will fail.
	CHECK(moveObjects(k));

	// Try to find all objects in the map:
	for (nat i = 0; i < count; i++) {
		CHECK_OBJ_EQ(map->get(k->at(i)), v->at(i));
	}

	// Move the objects once more, so we can try 'remove' properly.
	CHECK(moveObjects(k));

	// Try to remove an object.
	for (nat i = 0; i < count; i++) {
		map->remove(k->at(i));
		CHECK_EQ(map->count(), count - i - 1);

		// Validate the others.
		for (nat j = 0; j < count; j++) {
			if (j <= i) {
				CHECK(!map->has(k->at(j)));
			} else {
				CHECK_OBJ_EQ(map->get(k->at(j)), v->at(j));
			}
		}
	}

} END_TEST
