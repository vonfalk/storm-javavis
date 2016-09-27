#include "stdafx.h"
#include "Test/Lib/Test.h"
#include "Fn.h"

BEGIN_TEST(LoopTest) {
	Engine &e = *gEngine;

	CHECK_EQ(runFnInt(L"test.bs-simple.loop1"), 1024);
	CHECK_EQ(runFnInt(L"test.bs-simple.loop2"), 1023);
	CHECK_EQ(runFnInt(L"test.bs-simple.loop3"), 1024);
	CHECK_EQ(runFnInt(L"test.bs-simple.loop4"), 27);
	CHECK_EQ(runFnInt(L"test.bs-simple.loop5"), 27);

} END_TEST
