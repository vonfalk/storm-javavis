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

	// Check so that we have the same idea about which threads are which.
	CHECK_EQ(runFn<Thread *>(L"ui.testThread", 0), Compiler::thread(e));
	CHECK_NEQ(runFn<Thread *>(L"ui.testThread", 1), Compiler::thread(e));

	// Try to create an App instance and see if we crash.
	CHECK_RUNS(runFn<TObject *>(L"ui.app"));

} END_TEST
