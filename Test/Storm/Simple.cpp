#include "stdafx.h"
#include "Fn.h"

BEGIN_TEST(BasicSyntax, SimpleBS) {
	Engine &e = gEngine();

	CHECK_RUNS(runFn<Int>(L"test.bs-simple.voidFn"));
	CHECK_RUNS(runFn<Int>(L"test.bs-simple.emptyVoidFn"));

} END_TEST
