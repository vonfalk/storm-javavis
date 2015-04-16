#include "stdafx.h"
#include "Test/Test.h"
#include "Fn.h"
#include "Storm/Lib/StrBuf.h"
#include "Storm/Lib/CloneEnv.h"

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

	// Array toS...
	Auto<Array<Int>> arr = runFn<Array<Int> *>(L"test.bs.createIntArray");
	CHECK_EQ(::toS(arr), L"[1, 2, 3]");

	Auto<Array<Value>> valArr = runFn<Array<Value> *>(L"test.bs.createValueArray");
	valArr->push(value<StrBuf *>(*gEngine));
	valArr->push(value<CloneEnv *>(*gEngine));
	CHECK_EQ(::toS(valArr), L"[core.StrBuf, core.CloneEnv]");

} END_TEST
