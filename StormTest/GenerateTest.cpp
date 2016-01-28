#include "stdafx.h"
#include "Test/Test.h"
#include "Fn.h"

BEGIN_TEST_(GenerateTest) {

	CHECK_EQ(runFn<Int>(L"test.bs.genAdd", 10, 20), 30);

} END_TEST
