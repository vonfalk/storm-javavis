#include "stdafx.h"
#include "Test/Test.h"
#include "Fn.h"
#include "Storm/Lib/FnPtr.h"

static Int addTwo(Int v) {
	return v + 2;
}

static Int returnFive() {
	return 5;
}

BEGIN_TEST(FnPtrTest) {
	Engine &e = *gEngine;

	Auto<FnPtr<Int, Int>> fn = fnPtr(e, &addTwo);
	CHECK_EQ(fn->call(10), 12);

	Auto<FnPtr<Int>> voidFn = fnPtr(e, &returnFive);
	CHECK_EQ(voidFn->call(), 5);

	Auto<DbgActor> actor = CREATE(DbgActor, e);
	Auto<FnPtr<Str *, Par<Str>>> strFn = memberWeakPtr(e, actor, &DbgActor::echo);
	Auto<Str> original = CREATE(Str, e, L"A");
	Auto<Str> copy = strFn->call(original);
	CHECK(original.borrow() != copy.borrow());

	// Run some code in Storm!
	CHECK_EQ(runFnInt(L"test.bs.runFnPtr", fn), 12);
	CHECK_EQ(runFnInt(L"test.bs.runFnPtr", voidFn), 5);
	CHECK_EQ(runFnInt(L"test.bs.runStrFnPtr", strFn), 22);

	TODO(L"Test values here as well as verifying the cloning works as expected.");
} END_TEST
