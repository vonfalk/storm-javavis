#include "stdafx.h"
#include "Fn.h"

BEGIN_TEST_(Shared) {
	Engine &e = gEngine();

	// Try to load an external library.
	CHECK_EQ(runFn<Int>(L"ui.test", 10), 11);

	// Does the types get resolved properly?
	CHECK_EQ(runFn<Int>(L"ui.test", new (e) Str(L"A")), 1);

	// Does template classes work properly?
	Array<Int> *v = new (e) Array<Int>();
	v->push(1);
	v->push(2);
	v->push(3);
	CHECK_EQ(runFn<Int>(L"ui.test", v), 6);

} END_TEST
