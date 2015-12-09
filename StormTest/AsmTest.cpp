#include "stdafx.h"
#include "Test/Test.h"
#include "Fn.h"

BEGIN_TEST(AsmTest) {

	CHECK_EQ(runFn<Int>(L"test.asm.testInline"), 21);
	CHECK_EQ(runFn<Int>(L"test.asm.testInlineLabel"), 200);

} END_TEST
