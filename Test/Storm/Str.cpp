#include "stdafx.h"
#include "Fn.h"
#include "Compiler/Exception.h"

BEGIN_TEST(StrConcatTest, BS) {
	CHECK_EQ(::toS(runFn<Str *>(L"test.bs.strConcatTest")), L"ab1cvoid");
	CHECK_ERROR(runFn<Str *>(L"test.bs.strConcatError"), SyntaxError);
} END_TEST
