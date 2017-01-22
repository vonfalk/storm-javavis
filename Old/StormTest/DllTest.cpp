#include "stdafx.h"
#include "Test/Lib/Test.h"
#include "Fn.h"

BEGIN_TEST(DllTest) {
	// Test running something in the GUI dll!
	CHECK_OBJ_EQ(runFn<Str *>(L"test.bs.uiDllTest"), CREATE(Str, *gEngine, L"testABTest:1"));
} END_TEST
