#include "stdafx.h"
#include "Test/Lib/Test.h"
#include "Fn.h"

BEGIN_TEST_FN(checkTimes, const String &name, nat times) {
	DbgVal::clear();
	CHECK_RUNS(runFn<Int>(name, Int(0)));
	CHECK(DbgVal::clear());
	for (nat i = 0; i < times; i++) {
		CHECK_ERROR(runFn<Int>(name, Int(i + 1)), DebugError);
		CHECK(DbgVal::clear());
	}
	CHECK_RUNS(runFn<Int>(name, Int(times + 1)));
	CHECK(DbgVal::clear());
} END_TEST_FN

// Tests that checks the exception safety at various times in the generated code. Especially
// with regards to values.
BEGIN_TEST(ErrorTest) {
	CALL_TEST_FN(checkTimes, L"test.bs.basicException", 7);
	CALL_TEST_FN(checkTimes, L"test.bs.fnException", 3);
	CALL_TEST_FN(checkTimes, L"test.bs.threadException", 4);
	CALL_TEST_FN(checkTimes, L"test.bs.ctorError", 8);
} END_TEST
