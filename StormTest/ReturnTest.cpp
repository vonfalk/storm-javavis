#include "stdafx.h"
#include "Test/Test.h"
#include "Fn.h"

BEGIN_TEST(ReturnTest) {

	CHECK_EQ(runFn<Int>(L"test.bs.returnInt", 10), 10);
	CHECK_EQ(runFn<Int>(L"test.bs.returnInt", 40), 20);

} END_TEST
