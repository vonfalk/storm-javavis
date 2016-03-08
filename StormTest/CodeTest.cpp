#include "stdafx.h"
#include "Test/Test.h"
#include "Storm/Function.h"
#include "Code/FnParams.h"
#include "Fn.h"
#include "Shared/Timing.h"

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

BEGIN_TEST(CombinedTest) {
	CHECK_EQ(runFn<Int>(L"test.bs.combOp1"), 24);
	CHECK_EQ(runFn<Int>(L"test.bs.combOp2"), 24);
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
	CHECK_EQ(runFn<Float>(L"test.bs.promoteCtor"), 2);
	CHECK_EQ(runFn<Float>(L"test.bs.promoteInit"), 8);
	CHECK_EQ(runFn<Nat>(L"test.bs.initNat"), 20);
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
	CHECK_EQ(runFn<Int>(L"test.bs.testAsExpr2", 0), 30);
	CHECK_EQ(runFn<Int>(L"test.bs.testAsExpr2", 1), 12);
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

BEGIN_TEST(StormCtorTest) {
	CHECK_EQ(runFn<Int>(L"test.bs.ctorTest"), 50);
	CHECK_EQ(runFn<Int>(L"test.bs.ctorTest", 10), 30);
	CHECK_EQ(runFn<Int>(L"test.bs.ctorTestDbg", 10), 30);
	CHECK_RUNS(runFn<void>(L"test.bs.ignoreCtor"));
	CHECK_EQ(runFn<Int>(L"test.bs.ctorDerTest", 2), 6);
	CHECK_ERROR(runFn<Int>(L"test.bs.ctorErrorTest"), CodeError);
	CHECK_ERROR(runFn<Int>(L"test.bs.memberAssignErrorTest"), CodeError);
	CHECK_EQ(runFn<Int>(L"test.bs.testDefaultCtor"), 60);
} END_TEST

BEGIN_TEST(StrConcatTest) {
	CHECK_OBJ_EQ(runFn<Str *>(L"test.bs.strConcatTest"), CREATE(Str, *gEngine, L"ab1cvoid"));
	CHECK_ERROR(runFn<Str *>(L"test.bs.strConcatError"), SyntaxError);
} END_TEST

BEGIN_TEST(MaybeTest) {
	CHECK_EQ(runFn<Int>(L"test.bs.testMaybe", 0), 0);
	CHECK_EQ(runFn<Int>(L"test.bs.testMaybe", 1), 2);
	CHECK_EQ(runFn<Int>(L"test.bs.testMaybe", 2), 6);

	CHECK_EQ(runFn<Int>(L"test.bs.assignMaybe"), 1);

	CHECK_OBJ_EQ(runFn<Str *>(L"test.bs.maybeToS", 0), CREATE(Str, *gEngine, L"null"));
	CHECK_OBJ_EQ(runFn<Str *>(L"test.bs.maybeToS", 1), CREATE(Str, *gEngine, L"ok"));

	CHECK_EQ(runFn<Int>(L"test.bs.maybeInheritance"), 10);

	CHECK_EQ(runFn<Int>(L"test.bs.testMaybeInv", 0), 10);
	CHECK_EQ(runFn<Int>(L"test.bs.testMaybeInv", 1), 1);

	CHECK_EQ(runFn<Int>(L"test.bs.testMaybeInv2", 0), 10);
	CHECK_EQ(runFn<Int>(L"test.bs.testMaybeInv2", 1), 1);
} END_TEST

BEGIN_TEST(ScopeTest) {
	CHECK_EQ(runFn<Int>(L"test.bs.testScopeCls"), 10);
	CHECK_EQ(runFn<Int>(L"test.bs.testClassMember"), 20);
	CHECK_EQ(runFn<Int>(L"test.bs.testClassNonmember"), 20);
} END_TEST

BEGIN_TEST(CompileTest) {
	// Compile all code in core and lang. Not everything in test is supposed to compile cleanly.
	CHECK_RUNS(gEngine->package(L"core")->compile());
	CHECK_RUNS(gEngine->package(L"lang")->compile());
} END_TEST

BEGIN_TEST(UnitTest) {
	CHECK_EQ(runFn<Duration>(L"test.bs.testUnit"), ms(1) + s(1));
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
