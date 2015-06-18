#include "stdafx.h"
#include "Test/Test.h"
#include "Storm/Function.h"
#include "Code/FnParams.h"
#include "Fn.h"

#include "Storm/Basic/BSType.h"


BEGIN_TEST(BasicSyntax) {
	// These tests are placed separatly, making it easier to debug core problems that causes
	// syntax extensions to not work and therefore complicate debugging.
	CHECK_RUNS(runFn<Int>(L"test.bs-simple.voidFn"));
	CHECK_RUNS(runFn<Int>(L"test.bs-simple.emptyVoidFn"));

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

	CHECK_EQ(runFn<Int>(L"test.bs-simple.testStr"), 12);
	CHECK_ERROR(runFn<Int>(L"test.bs-simple.forError", 10), DebugError);
	CHECK_RUNS(runFn<Int>(L"test.bs-simple.forScope"));
	CHECK_RUNS(runFn<Int>(L"test.bs-simple.forScopeVal"));
} END_TEST

BEGIN_TEST(PriorityTest) {
	CHECK_EQ(runFn<Int>(L"test.bs.prio1"), 203);
	CHECK_EQ(runFn<Int>(L"test.bs.prio2"), 203);
	CHECK_EQ(runFn<Int>(L"test.bs.prio3"), 230);
	CHECK_EQ(runFn<Int>(L"test.bs.prio4"), 230);
	CHECK_EQ(runFn<Int>(L"test.bs.prio5"), 6010);
	CHECK_EQ(runFn<Int>(L"test.bs.prio6"), 11);
	CHECK_EQ(runFn<Int>(L"test.bs.prio7"), 0);
	CHECK_EQ(runFn<Int>(L"test.bs.prio8"), 2);
	CHECK_EQ(runFn<Int>(L"test.bs.prio9"), 2);
} END_TEST

BEGIN_TEST(OverloadTest) {
	// If this fails, the compiler does not choose the correct overload in the case of ambiguities.
	CHECK_EQ(runFn<Int>(L"test.bs.testOverload", 1), 1);
	CHECK_EQ(runFn<Int>(L"test.bs.testOverload", 0), 2);
} END_TEST

BEGIN_TEST(AutocastTest) {
	// Check auto-casting from int to nat.
	CHECK_EQ(runFn<Int>(L"test.bs.castToNat"), 20);
	CHECK_EQ(runFn<Int>(L"test.bs.castToMaybe"), 20);
	CHECK_EQ(runFn<Int>(L"test.bs.downcastMaybe"), 20);
	CHECK_RUNS(runFn<Int>(L"test.bs.ifCast"));
	CHECK_EQ(runFn<Int>(L"test.bs.autoCast", 5), 10);
} END_TEST

BEGIN_TEST(InheritanceTest) {
	// Some inheritance testing.
	CHECK_EQ(runFn<Int>(L"test.bs.testBase"), 10);
	CHECK_EQ(runFn<Int>(L"test.bs.testDerived"), 20);

	// Using C++ classes as base.
	CHECK_EQ(runFn<Int>(L"test.bs.testCpp", 1), 10);
	CHECK_EQ(runFn<Int>(L"test.bs.testCpp", 2), 20);

	Auto<Object> created = runFn<Object*>(L"test.bs.createCpp", 1);
	CHECK_EQ(created.expect<Dbg>(*gEngine, L"dbg")->get(), 10);
	created = runFn<Object*>(L"test.bs.createCpp", 2);
	CHECK_EQ(created.expect<Dbg>(*gEngine, L"dbg")->get(), 20);

	// Use of 'super'.
	CHECK_EQ(runFn<Int>(L"test.bs.testSuperExpr"), 30);

	// Use of 'as'.
	CHECK_EQ(runFn<Int>(L"test.bs.testAsExpr", 0), 30);
	CHECK_EQ(runFn<Int>(L"test.bs.testAsExpr", 1), 12);
} END_TEST

BEGIN_TEST(ValueTest) {
	// Values.
	DbgVal::clear();
	CHECK_EQ(runFn<Int>(L"test.bs.testValue"), 10);
	CHECK(DbgVal::clear());
	CHECK_EQ(runFn<Int>(L"test.bs.testDefInit"), 10);
	CHECK(DbgVal::clear());
	CHECK_EQ(runFn<Int>(L"test.bs.testValAssign"), 20);
	CHECK(DbgVal::clear());
	CHECK_EQ(runFn<Int>(L"test.bs.testValCopy"), 20);
	CHECK(DbgVal::clear());
	CHECK_EQ(runFn<Int>(L"test.bs.testValCtor"), 7);
	CHECK(DbgVal::clear());
	CHECK_EQ(runFn<Int>(L"test.bs.testValParam"), 16);
	CHECK(DbgVal::clear());
	CHECK_EQ(runFn<Int>(L"test.bs.testValReturn"), 22);
	CHECK(DbgVal::clear());
	CHECK_EQ(runFn<DbgVal>(L"test.bs.createVal", 20), DbgVal(20));
	CHECK(DbgVal::clear());
	CHECK_EQ((runFn<Int, DbgVal>(L"test.bs.asVal", DbgVal(11))), 13);
	CHECK(DbgVal::clear());
	CHECK_EQ(runFn<Int>(L"test.bs.testContainVal"), 10);
	CHECK(DbgVal::clear());
	CHECK_EQ(runFn<Int>(L"test.bs.testContainVal"), 10);
	CHECK(DbgVal::clear());
} END_TEST

BEGIN_TEST(CustomValueTest) {
	CHECK_EQ(runFn<Int>(L"test.bs.testCustomValue"), -300);
	CHECK_EQ(runFn<Int>(L"test.bs.testRefVal", 24), 24);
	CHECK_EQ(runFn<Int>(L"test.bs.testCopyRefVal", 24), 24);
	CHECK_EQ(runFn<Int>(L"test.bs.testAssignRefVal", 24), 24);
	CHECK_EQ(runFn<Int>(L"test.bs.testValVal", 22), 22);
	CHECK_EQ(runFn<Int>(L"test.bs.testCopyValVal", 22), 22);
	CHECK_EQ(runFn<Int>(L"test.bs.testAssignValVal", 22), 22);
} END_TEST

BEGIN_TEST(ValueMemberTest) {
	CHECK_EQ(runFn<Int>(L"test.bs.testVirtualVal1"), 10);
	CHECK_EQ(runFn<Int>(L"test.bs.testVirtualVal2"), 20);
	CHECK_EQ(runFn<Int>(L"test.bs.testVirtualVal3"), 15);
	// This test is really good in release builts. For VS2008, the compiler uses
	// the return value (in eax) of v->asDbgVal(), and crashes if we fail to return
	// a correct value. In debug builds, the compiler usually re-loads the value
	// itself instead.
	Auto<Dbg> v = runFn<Dbg *>(L"test.bs.testVirtualVal4");
	CHECK_EQ(v->asDbgVal().v, 20);

	// Does the thread thunks correctly account for the special handling of member functions?
	CHECK_EQ(runFn<Int>(L"test.bs.testActorVal"), 10);
} END_TEST

BEGIN_TEST(StormArrayTest) {
	CHECK_EQ(runFn<Int>(L"test.bs.testArray"), 230);
	CHECK_EQ(runFn<Int>(L"test.bs.testValArray"), 250);
	CHECK_EQ(runFn<Int>(L"test.bs.testIntArray"), 95);
	CHECK_EQ(runFn<Int>(L"test.bs.testInitArray"), 1337);
	CHECK_EQ(runFn<Int>(L"test.bs.testInitAutoArray"), 1234);
	CHECK_EQ(runFn<Int>(L"test.bs.testAutoArray"), 0);
	CHECK_EQ(runFn<Int>(L"test.bs.testCastArray"), 2);

	// Interoperability.
	Auto<Array<DbgVal>> r = runFn<Array<DbgVal>*>(L"test.bs.createValArray");
	CHECK_EQ(r->count(), 20);
	for (nat i = 0; i < 20; i++) {
		CHECK_EQ(r->at(i).get(), i);
	}

} END_TEST

BEGIN_TEST(StormCtorTest) {
	CHECK_EQ(runFn<Int>(L"test.bs.ctorTest"), 50);
	CHECK_EQ(runFn<Int>(L"test.bs.ctorTest", 10), 30);
	CHECK_EQ(runFn<Int>(L"test.bs.ctorTestDbg", 10), 30);
	CHECK_EQ(runFn<Int>(L"test.bs.ctorDerTest", 2), 6);
	CHECK_ERROR(runFn<Int>(L"test.bs.ctorErrorTest"), CodeError);
	CHECK_ERROR(runFn<Int>(L"test.bs.memberAssignErrorTest"), CodeError);
	CHECK_EQ(runFn<Int>(L"test.bs.testDefaultCtor"), 60);
} END_TEST

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
} END_TEST

BEGIN_TEST(StrConcatTest) {
	CHECK_OBJ_EQ(runFn<Str *>(L"test.bs.strConcatTest"), CREATE(Str, *gEngine, L"ab1cvoid"));
	CHECK_ERROR(runFn<Str *>(L"test.bs.strConcatError"), SyntaxError);
} END_TEST

BEGIN_TEST(MaybeTest) {
	CHECK_EQ(runFn<Int>(L"test.bs.testMaybe", 0), 0);
	CHECK_EQ(runFn<Int>(L"test.bs.testMaybe", 1), 1);
	CHECK_EQ(runFn<Int>(L"test.bs.testMaybe", 2), 4);

	CHECK_EQ(runFn<Int>(L"test.bs.assignMaybe"), 1);

	CHECK_OBJ_EQ(runFn<Str *>(L"test.bs.maybeToS", 0), CREATE(Str, *gEngine, L"null"));
	CHECK_OBJ_EQ(runFn<Str *>(L"test.bs.maybeToS", 1), CREATE(Str, *gEngine, L"ok"));

	CHECK_EQ(runFn<Int>(L"test.bs.maybeInheritance"), 10);
} END_TEST

// Test the REPL of BS programmatically.
BEGIN_TESTX(ReplTest) {
	runFn<void>(L"test.bs.replTest");
} END_TEST


BEGIN_TESTX(BFTest) {
	// Takes a long time to run. Mostly here for testing.
	runFn<Int>(L"test.bf.separateBf");
	runFn<Int>(L"test.bf.inlineBf");

} END_TEST
