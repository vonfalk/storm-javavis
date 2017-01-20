#include "stdafx.h"
#include "Fn.h"

BEGIN_TEST(Asm, BS) {
	CHECK_EQ(runFn<Int>(L"test.asm.testInline"), 21);
	CHECK_EQ(runFn<Int>(L"test.asm.testInlineLabel"), 200);
} END_TEST
