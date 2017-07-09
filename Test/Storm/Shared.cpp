#include "stdafx.h"
#include "Fn.h"

BEGIN_TEST(Shared, Storm) {
	Engine &e = gEngine();

	// Try to load an external library.
	CHECK_EQ(runFn<Int>(S("ui.test"), 10), 11);

	// Does the types get resolved properly?
	CHECK_EQ(runFn<Int>(S("ui.test"), new (e) Str(S("A"))), 1);

	// Does template classes work properly?
	Array<Int> *v = new (e) Array<Int>();
	v->push(1);
	v->push(2);
	v->push(3);
	CHECK_EQ(runFn<Int>(S("ui.test"), v), 6);

	// Check so that we have the same idea about which threads are which.
	CHECK_EQ(runFn<Thread *>(S("ui.testThread"), 0), Compiler::thread(e));
	CHECK_NEQ(runFn<Thread *>(S("ui.testThread"), 1), Compiler::thread(e));

	// Try to create an App instance and see if we crash.
	CHECK_RUNS(runFn<TObject *>(S("ui.app")));

} END_TEST
