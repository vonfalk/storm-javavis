#include "stdafx.h"
#include "Test/Test.h"
#include "Fn.h"

/**
 * Test interaction between Storm:s toS and C++ toS.
 */
BEGIN_TEST(ToSTest) {
	Auto<Dbg> a = runFn<Dbg *>(L"test.bs.toSDbg", 0);
	Auto<Dbg> b = runFn<Dbg *>(L"test.bs.toSDbg", 1);

	// If this does not work, overriding functions is broken.
	Auto<Str> v = b->toS();
	CHECK_EQ(v->v, L"1");

	// This should call the overridden function in Storm.
	CHECK_EQ(::toS(b), L"1");

} END_TEST
