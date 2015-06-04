#include "stdafx.h"
#include "Test/Test.h"
#include "Fn.h"

BEGIN_TEST_(DllTest) {
	// Test running something in the GUI dll!

	runFn(L"test.bs.uiDllTest");
} END_TEST
