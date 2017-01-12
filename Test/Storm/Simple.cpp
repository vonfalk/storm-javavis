#include "stdafx.h"
#include "Fn.h"

BEGIN_TEST_(BasicSyntax, SimpleBS) {
	Engine &e = gEngine();

	CHECK_RUNS(runFn<void>(L"test.bs-simple.voidFn"));
	CHECK_RUNS(runFn<void>(L"test.bs-simple.emptyVoidFn"));

	CHECK_EQ(runFn<Int>(L"test.bs-simple.bar"), 3);
	CHECK_EQ(runFn<Int>(L"test.bs-simple.ifTest", 1), 3);
	CHECK_EQ(runFn<Int>(L"test.bs-simple.ifTest", 2), 4);
	CHECK_EQ(runFn<Int>(L"test.bs-simple.ifTest", 3), 5);

} END_TEST
