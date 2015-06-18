#include "stdafx.h"
#include "Test/Test.h"
#include "Fn.h"

BEGIN_TEST(FloatTest) {

	CHECK_EQ(runFn<Float>(L"test.bs.floatParams", 10.2f, 1.0f), 10.2f);
	CHECK_EQ(runFn<Float>(L"test.bs.floatAdd", 10.2f, 1.0f), 11.2f);
	CHECK_EQ(runFn<Float>(L"test.bs.floatSub", 10.2f, 1.0f), 9.2f);
	CHECK_EQ(runFn<Float>(L"test.bs.floatMul", 10.2f, 2.0f), 20.4f);
	CHECK_EQ(runFn<Float>(L"test.bs.floatDiv", 10.2f, 2.0f), 5.1f);
	CHECK_EQ(runFn<Float>(L"test.bs.floatLiteral"), 11.5f);

	CHECK_EQ(runFn<Float>(L"test.bs.floatThread"), 11.5f);
	CHECK_EQ(runFn<Float>(L"test.bs.floatFuture"), 12.5f);

	CHECK_EQ(runFnInt(L"test.bs.floatRound", 1.6f), 1);
	CHECK_EQ(runFnInt(L"test.bs.floatRound", 1.5f), 1);
	CHECK_EQ(runFnInt(L"test.bs.floatRound", 1.4f), 1);

} END_TEST
