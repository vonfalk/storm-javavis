#include "stdafx.h"
#include "Test/Test.h"
#include "Fn.h"

BEGIN_TEST(GenerateTest) {

	CHECK_EQ(runFn<Int>(L"test.bs.genAdd", 10, 20), 30);
	CHECK_EQ(runFn<Float>(L"test.bs.genAdd", 10.2f, 20.3f), 30.5f);

	CHECK_EQ(runFn<Int>(L"test.bs.testGenClass", 10), 12);

} END_TEST
