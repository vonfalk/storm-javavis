#include "stdafx.h"
#include "Fn.h"

BEGIN_TEST_(BasicSyntax, SimpleBS) {
	Engine &e = gEngine();

	CHECK_RUNS(runFn<void>(L"test.bs-simple.voidFn"));
	CHECK_RUNS(runFn<void>(L"test.bs-simple.emptyVoidFn"));

} END_TEST
