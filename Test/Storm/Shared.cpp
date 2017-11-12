#include "stdafx.h"
#include "Fn.h"

BEGIN_TEST(Shared, Storm) {
	Engine &e = gEngine();

	// Try to load an external library.
	CHECK_EQ(runFn<Int>(S("test.shared.test"), 10), 11);

	// Does the types get resolved properly?
	CHECK_EQ(runFn<Int>(S("test.shared.test"), new (e) Str(S("A"))), 1);

	// Does template classes work properly?
	Array<Int> *v = new (e) Array<Int>();
	v->push(1);
	v->push(2);
	v->push(3);
	CHECK_EQ(runFn<Int>(S("test.shared.test"), v), 6);

	// Check so that we have the same idea about which threads are which.
	CHECK_EQ(runFn<Thread *>(S("test.shared.testThread"), 0), Compiler::thread(e));
	CHECK_NEQ(runFn<Thread *>(S("test.shared.testThread"), 1), Compiler::thread(e));

	// Try to create some global data in the shared library!
	CHECK_EQ(toS(runFn<Object *>(S("test.shared.global"))), L"1");
	CHECK_EQ(toS(runFn<Object *>(S("test.shared.global"))), L"2");

} END_TEST
