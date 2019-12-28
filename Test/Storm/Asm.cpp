#include "stdafx.h"
#include "Fn.h"

BEGIN_TEST(Asm, BS) {
	CHECK_EQ(runFn<Int>(S("tests.asm.testInline")), 21);
	CHECK_EQ(runFn<Int>(S("tests.asm.testInlineLabel")), 200);
} END_TEST
