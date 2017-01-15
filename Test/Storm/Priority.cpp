#include "stdafx.h"
#include "Fn.h"

BEGIN_TEST_(Priority) {
	CHECK_EQ(runFn<Int>(L"test.bs.prio1"), 203);
	CHECK_EQ(runFn<Int>(L"test.bs.prio2"), 203);
	CHECK_EQ(runFn<Int>(L"test.bs.prio3"), 230);
	CHECK_EQ(runFn<Int>(L"test.bs.prio4"), 230);
	CHECK_EQ(runFn<Int>(L"test.bs.prio5"), 6010);
	CHECK_EQ(runFn<Int>(L"test.bs.prio6"), 11);
	CHECK_EQ(runFn<Int>(L"test.bs.prio7"), 0);
	CHECK_EQ(runFn<Int>(L"test.bs.prio8"), 2);
	CHECK_EQ(runFn<Int>(L"test.bs.prio9"), 2);

	CHECK_EQ(runFn<Int>(L"test.bs.combOp1"), 24);
	CHECK_EQ(runFn<Int>(L"test.bs.combOp2"), 24);
} END_TEST
