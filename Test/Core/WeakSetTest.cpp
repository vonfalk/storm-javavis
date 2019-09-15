#include "stdafx.h"
#include "Core/WeakSet.h"
#include "Core/Str.h"
#include "Compiler/Debug.h"

using debug::PtrKey;

static bool setFind(WeakSet<PtrKey> *in, PtrKey *elem) {
	WeakSet<PtrKey>::Iter i = in->iter();
	while (TObject *o = i.next())
		if (o == elem)
			return true;

	return false;
}

static nat setCount(WeakSet<PtrKey> *in) {
	nat c = 0;
	WeakSet<PtrKey>::Iter i = in->iter();
	while (i.next())
		c++;

	return c;
}

BEGIN_TEST(WeakSetTest, Core) {
	Engine &e = gEngine();

	WeakSet<PtrKey> *set = new (e) WeakSet<PtrKey>();

	PtrKey *lone = new (e) PtrKey();
	set->put(lone);

	for (nat i = 0; i < 100; i++)
		set->put(new (e) PtrKey());

	CHECK(set->any());
	CHECK(set->has(lone));
	CHECK(setFind(set, lone));

	// Try to make the gc collect the weak references in the set.
	e.gc.collect();

	for (nat i = 0; i < 1000; i++)
		new (e) PtrKey();

	e.gc.collect();

	CHECK(setFind(set, lone));
	// Should have forgot some of the elements by now.
	CHECK(setCount(set) < 100);

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

static bool validate(Array<PtrKey *> *k, WeakSet<PtrKey> *set) {
	for (nat i = 0; i < k->count(); i++) {
		if (!set->has(k->at(i))) {
			PLN(L"Missing object " << (void *)k->at(i));
			return false;
		}
	}

	return true;
}


BEGIN_TEST(WeakSetStress, Stress) {
	Engine &e = gEngine();
	const nat count = 500;
	const nat times = 1000;

	Array<PtrKey *> *k = new (e) Array<PtrKey *>();
	for (nat i = 0; i < count; i++)
		k->push(new (e) PtrKey());

	WeakSet<PtrKey> *set = new (e) WeakSet<PtrKey>();

	for (nat i = 0; i < count; i++) {
		set->put(k->at(i));
	}

	for (nat i = 0; i < times; i++) {
		moveObjects(k);

		// See if we have any garbage.
		{
			nat count = 0;
			WeakSet<PtrKey>::Iter iter = set->iter();
			while (iter.next())
				count++;

			CHECK_GTE(count, k->count());
			CHECK_LTE(count, 5*k->count());
		}

		{
			// Swap some elements.
			for (nat j = count/2; j < count; j++)
				k->at(j) = new (e) PtrKey();
			for (nat j = 0; j < count; j++)
				set->put(k->at(j));
		}

		CHECK(validate(k, set));

		// Otherwise, something is very wrong!
		CHECK_LTE(set->capacity(), 100*count);
	}

} END_TEST


BEGIN_TEST(WeakSetRemoveStress, Stress) {
	Engine &e = gEngine();
	const nat count = 500;
	const nat times = 1000;

	Array<PtrKey *> *k = new (e) Array<PtrKey *>();
	for (nat i = 0; i < count; i++)
		k->push(new (e) PtrKey());

	WeakSet<PtrKey> *set = new (e) WeakSet<PtrKey>();

	for (nat i = 0; i < count; i++) {
		set->put(k->at(i));
	}

	for (nat i = 0; i < times; i++) {
		moveObjects(k);

		// See if we have any garbage.
		{
			nat count = 0;
			WeakSet<PtrKey>::Iter iter = set->iter();
			while (iter.next())
				count++;

			CHECK_GTE(count, k->count());
			CHECK_LTE(count, 5*k->count());
		}

		{
			// Swap some elements.
			for (nat j = count/2; j < count; j++) {
				set->remove(k->at(j));
				k->at(j) = new (e) PtrKey();
			}
			for (nat j = 0; j < count; j++)
				set->put(k->at(j));
		}

		CHECK(validate(k, set));

		// Otherwise, something is very wrong!
		CHECK_LTE(set->capacity(), 100*count);
	}

} END_TEST
