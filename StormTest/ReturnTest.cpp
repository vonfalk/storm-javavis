#include "stdafx.h"
#include "Test/Lib/Test.h"
#include "Fn.h"

BEGIN_TEST(ReturnTest) {

	// Return integers.
	CHECK_EQ(runFn<Int>(L"test.bs.returnInt", 10), 10);
	CHECK_EQ(runFn<Int>(L"test.bs.returnInt", 40), 20);

	// Return strings.
	CHECK_EQ(steal(runFn<Str *>(L"test.bs.returnStr", 10))->v, L"no");
	CHECK_EQ(steal(runFn<Str *>(L"test.bs.returnStr", 40))->v, L"40");

	// Return values.
	DbgVal::clear();
	CHECK_EQ(runFn<DbgVal>(L"test.bs.returnDbgVal", 11).get(), 10);
	CHECK_EQ(runFn<DbgVal>(L"test.bs.returnDbgVal", 30).get(), 20);
	CHECK(DbgVal::clear());

	// Return type checking, interaction with 'no-return' returns.
	CHECK_EQ(runFn<Int>(L"test.bs.returnAlways", 22), 22);
	CHECK_EQ(runFn<Int>(L"test.bs.deduceType", 21), 22);
	CHECK_EQ(runFn<Int>(L"test.bs.prematureReturn", 20), 30);

} END_TEST
