#include "stdafx.h"
#include "Test/Test.h"
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
	while (TObject *o = i.next())
		c++;

	return c;
}

BEGIN_TEST(WeakSetTest, Runtime) {
	Engine &e = *gEngine;

	WeakSet<PtrKey> *set = new (e) WeakSet<PtrKey>();

	PtrKey *lone = new (e) PtrKey();
	set->put(lone);

	for (nat i = 0; i < 10; i++)
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
	CHECK(setCount(set) < 10);

} END_TEST
