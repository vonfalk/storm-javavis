#include "stdafx.h"
#include "Test/Test.h"
#include "Fn.h"

BEGIN_TEST(DllTest) {
	// Test running something in the GUI dll!
	CHECK_OBJ_EQ(runFn<Str *>(L"test.bs.uiDllTest"), CREATE(Str, *gEngine, L"testAB"));
} END_TEST
