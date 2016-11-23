#include "stdafx.h"
#include "Core/Map.h"
#include "Core/Str.h"
#include "Core/Hash.h"
#include "Compiler/Debug.h"

using debug::PtrKey;


BEGIN_TEST(MapTest, Core) {
	Engine &e = gEngine();

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
		CHECK_EQ(::toS(map->get(new (e) Str(L"A"), new (e) Str(L"-"))), L"-");
		CHECK_ERROR(::toS(map->get(new (e) Str(L"A"))), MapError);
	}

	// Do primitives work as values?
	{
		Map<Str *, Int> *map = new (e) Map<Str *, Int>();

		map->put(new (e) Str(L"A"), 10);
		map->put(new (e) Str(L"B"), 11);
		map->put(new (e) Str(L"A"), 12);
		map->put(new (e) Str(L"E"), 13);

		CHECK_EQ(map->count(), 3);
		CHECK_EQ(map->get(new (e) Str(L"A")), 12);

		map->remove(new (e) Str(L"A"));

		CHECK_EQ(map->count(), 2);
		CHECK_EQ(map->get(new (e) Str(L"B")), 11);
		CHECK_EQ(map->get(new (e) Str(L"E")), 13);
		CHECK_EQ(map->get(new (e) Str(L"A"), 99), 99);
		CHECK_ERROR(map->get(new (e) Str(L"A")), MapError);
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

BEGIN_TEST(MapTestMove, Core) {
	// Do we handle moving objects properly?
	Engine &e = gEngine();

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

static bool verify(Array<PtrKey *> *k, Array<Str *> *v, Map<PtrKey *, Str *> *map) {
	if (!moveObjects(k))
		return false;

	for (nat i = 0; i < k->count(); i++) {
		Str *found = map->get(k->at(i), null);
		if (found != v->at(i)) {
			PLN(L"Failed: " << (void *)k->at(i) << L" (" << (void *)ptrHash(k->at(i)) << L"), got " << found << L", expected " << v->at(i));
			map->dbg_print();
			return false;
		}
	}

	return true;
}

BEGIN_TEST(MapTestMoveStress, Stress) {
	Engine &e = gEngine();
	const nat count = 500;
	const nat times = 1000;

	Array<PtrKey *> *k = new (e) Array<PtrKey *>();
	Array<Str *> *v = new (e) Array<Str *>();

	for (nat i = 0; i < count; i++) {
		k->push(new (e) PtrKey());
		v->push(new (e) Str(::toS(i).c_str()));
	}

	Map<PtrKey *, Str *> *map = new (e) Map<PtrKey *, Str *>();

	for (nat i = 0; i < count; i++) {
		map->put(k->at(i), v->at(i));
	}

	for (nat i = 0; i < times; i++) {
		bool z = verify(k, v, map);
		CHECK(z);
		if (!z)
			break;
	}
} END_TEST
