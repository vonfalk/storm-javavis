#include "stdafx.h"
#include "Fn.h"
#include "Compiler/Exception.h"


BEGIN_TEST_(CatchTest, BS) {
	CHECK_EQ(::toS(runFn<Str *>(S("tests.bs.catchTest"), 0)), L"None");
	CHECK_EQ(::toS(runFn<Str *>(S("tests.bs.catchTest"), 1)), L"DDebug error");
	CHECK_EQ(::toS(runFn<Str *>(S("tests.bs.catchTest"), 2)), L"S@<unknown location>: Syntax error: Test");
} END_TEST

BEGIN_TEST_(ThrowTest, BS) {
	try {
		runFn<Int>(S("tests.bs.throwTest"));
		CHECK(false);
	} catch (const SyntaxError *e) {
		// Make sure it actually contains something useful!
		CHECK_EQ(::toS(e), L"@<unknown location>: Syntax error: Test");
	} catch (...) {
		CHECK(false);
	}
} END_TEST
