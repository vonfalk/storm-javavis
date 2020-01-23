#include "stdafx.h"
#include "Fn.h"


/**
 * Using a Variant from within Storm.
 */
BEGIN_TEST_(CatchTest, BS) {
	CHECK_EQ(::toS(runFn<Str *>(S("tests.bs.catchTest"), 0)), L"None");
	CHECK_EQ(::toS(runFn<Str *>(S("tests.bs.catchTest"), 1)), L"DDebug error");
	CHECK_EQ(::toS(runFn<Str *>(S("tests.bs.catchTest"), 2)), L"S@<unknown location>: Syntax error: Test");
} END_TEST
