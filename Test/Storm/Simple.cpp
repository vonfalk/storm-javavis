#include "stdafx.h"
#include "Fn.h"
#include "Compiler/Exception.h"

BEGIN_TEST(BasicSyntax, SimpleBS) {
	Engine &e = gEngine();

	CHECK_RUNS(runFn<void>(L"test.bs-simple.voidFn"));
	CHECK_RUNS(runFn<void>(L"test.bs-simple.emptyVoidFn"));

	CHECK_EQ(runFn<Int>(L"test.bs-simple.bar"), 3);
	CHECK_EQ(runFn<Int>(L"test.bs-simple.ifTest", 1), 3);
	CHECK_EQ(runFn<Int>(L"test.bs-simple.ifTest", 2), 4);
	CHECK_EQ(runFn<Int>(L"test.bs-simple.ifTest", 3), 5);
	CHECK_EQ(runFn<Int>(L"test.bs-simple.ifTest2", 3), 4);
	CHECK_EQ(runFn<Int>(L"test.bs-simple.ifTest2", 0), -1);
	CHECK_EQ(runFn<Int>(L"test.bs-simple.ifTest3", 0), 10);
	CHECK_EQ(runFn<Int>(L"test.bs-simple.ifTest3", 1), 20);
	CHECK_EQ(runFn<Int>(L"test.bs-simple.ifTest4", 0), 10);
	CHECK_EQ(runFn<Int>(L"test.bs-simple.ifTest4", 1), 20);
	CHECK_EQ(runFn<Int>(L"test.bs-simple.ifTest5", 1), 15);
	CHECK_EQ(runFn<Int>(L"test.bs-simple.ifTest5", 3), 14);

	CHECK_EQ(runFn<Int>(L"test.bs-simple.assign", 1), 2);
	CHECK_EQ(runFn<Int>(L"test.bs-simple.while", 10), 1024);
	CHECK_EQ(runFn<Int>(L"test.bs-simple.for", 10), 1024);

	CHECK_EQ(runFn<Int>(L"test.bs-simple.createFoo"), 3);

	CHECK_EQ(runFn<Int>(L"test.bs-simple.testCtor"), 20);
	CHECK_EQ(runFn<Int>(L"test.bs-simple.testIntCtor"), 20);

	CHECK_ERROR(runFn<Int>(L"test.bs-simple.forError", 10), DebugError);
	CHECK_RUNS(runFn<void>(L"test.bs-simple.forScope"));
	CHECK_RUNS(runFn<void>(L"test.bs-simple.forScopeVal"));

} END_TEST

BEGIN_TEST(LoopTest, SimpleBS) {
	CHECK_EQ(runFn<Int>(L"test.bs-simple.loop1"), 1024);
	CHECK_EQ(runFn<Int>(L"test.bs-simple.loop2"), 1023);
	CHECK_EQ(runFn<Int>(L"test.bs-simple.loop3"), 1024);
	CHECK_EQ(runFn<Int>(L"test.bs-simple.loop4"), 27);
	CHECK_EQ(runFn<Int>(L"test.bs-simple.loop5"), 27);
} END_TEST

