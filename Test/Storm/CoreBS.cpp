#include "stdafx.h"
#include "Fn.h"
#include "Compiler/Debug.h"
#include "Compiler/Exception.h"
#include "Compiler/Package.h"
#include "Core/Timing.h"

using storm::debug::DbgVal;
using storm::debug::Dbg;

BEGIN_TEST(Priority, BS) {
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

BEGIN_TEST(Overload, BS) {
	// If this fails, the compiler does not choose the correct overload in the case of ambiguities.
	CHECK_EQ(runFn<Int>(L"test.bs.testOverload", 1), 1);
	CHECK_EQ(runFn<Int>(L"test.bs.testOverload", 0), 2);
} END_TEST

BEGIN_TEST(InheritanceTest, BS) {
	using storm::debug::Dbg;

	// Some inheritance testing.
	CHECK_EQ(runFn<Int>(L"test.bs.testBase"), 10);
	CHECK_EQ(runFn<Int>(L"test.bs.testDerived"), 20);

	// Using C++ classes as base.
	CHECK_EQ(runFn<Int>(L"test.bs.testCpp", 1), 10);
	CHECK_EQ(runFn<Int>(L"test.bs.testCpp", 2), 20);

	Dbg *created = runFn<Dbg *>(L"test.bs.createCpp", 1);
	CHECK_EQ(created->get(), 10);
	created = runFn<Dbg *>(L"test.bs.createCpp", 2);
	CHECK_EQ(created->get(), 20);

	// Use of 'super'.
	CHECK_EQ(runFn<Int>(L"test.bs.testSuperExpr"), 30);

	// Use of 'as'.
	CHECK_EQ(runFn<Int>(L"test.bs.testAsExpr", 0), 30);
	CHECK_EQ(runFn<Int>(L"test.bs.testAsExpr", 1), 12);
	CHECK_EQ(runFn<Int>(L"test.bs.testAsExpr2", 0), 30);
	CHECK_EQ(runFn<Int>(L"test.bs.testAsExpr2", 1), 12);
} END_TEST

/**
 * Values.
 */

BEGIN_TEST(ValueTest, BS) {
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
	// TODO: See if we can test so that destructors are executed from within classes/actors.
} END_TEST

BEGIN_TEST(CustomValueTest, BS) {
	CHECK_EQ(runFn<Int>(L"test.bs.testCustomValue"), -300);
	CHECK_EQ(runFn<Int>(L"test.bs.testRefVal", 24), 24);
	CHECK_EQ(runFn<Int>(L"test.bs.testCopyRefVal", 24), 24);
	CHECK_EQ(runFn<Int>(L"test.bs.testAssignRefVal", 24), 24);
	CHECK_EQ(runFn<Int>(L"test.bs.testValVal", 22), 22);
	CHECK_EQ(runFn<Int>(L"test.bs.testCopyValVal", 22), 22);
	CHECK_EQ(runFn<Int>(L"test.bs.testAssignValVal", 22), 22);
} END_TEST

BEGIN_TEST(ValueMemberTest, BS) {
	CHECK_EQ(runFn<Int>(L"test.bs.testVirtualVal1"), 10);
	CHECK_EQ(runFn<Int>(L"test.bs.testVirtualVal2"), 20);
	CHECK_EQ(runFn<Int>(L"test.bs.testVirtualVal3"), 15);
	// This test is really good in release builts. For VS2008, the compiler uses
	// the return value (in eax) of v->asDbgVal(), and crashes if we fail to return
	// a correct value. In debug builds, the compiler usually re-loads the value
	// itself instead.
	Dbg *v = runFn<Dbg *>(L"test.bs.testVirtualVal4");
	CHECK_EQ(v->asDbgVal().v, 20);

	// Does the thread thunks correctly account for the special handling of member functions?
	TODO(L"Enable this test!");
	// CHECK_EQ(runFn<Int>(L"test.bs.testActorVal"), 10);
} END_TEST


/**
 * Autocast.
 */

BEGIN_TEST(AutocastTest, BS) {
	// Check auto-casting from int to nat.
	CHECK_EQ(runFn<Int>(L"test.bs.castToNat"), 20);
	CHECK_EQ(runFn<Int>(L"test.bs.castToMaybe"), 20);
	CHECK_EQ(runFn<Int>(L"test.bs.downcastMaybe"), 20);
	CHECK_RUNS(runFn<void>(L"test.bs.ifCast"));
	CHECK_EQ(runFn<Int>(L"test.bs.autoCast", 5), 10);
	CHECK_EQ(runFn<Float>(L"test.bs.promoteCtor"), 2);
	CHECK_EQ(runFn<Float>(L"test.bs.promoteInit"), 8);
	CHECK_EQ(runFn<Nat>(L"test.bs.initNat"), 20);
} END_TEST


/**
 * Maybe.
 */

BEGIN_TEST(MaybeTest, BS) {
	CHECK_EQ(runFn<Int>(L"test.bs.testMaybe", 0), 0);
	CHECK_EQ(runFn<Int>(L"test.bs.testMaybe", 1), 2);
	CHECK_EQ(runFn<Int>(L"test.bs.testMaybe", 2), 6);

	CHECK_EQ(runFn<Int>(L"test.bs.assignMaybe"), 1);

	CHECK_EQ(::toS(runFn<Str *>(L"test.bs.maybeToS", 0)), L"null");
	CHECK_EQ(::toS(runFn<Str *>(L"test.bs.maybeToS", 1)), L"ok");

	CHECK_EQ(runFn<Int>(L"test.bs.maybeInheritance"), 10);

	CHECK_EQ(runFn<Int>(L"test.bs.testMaybeInv", 0), 10);
	CHECK_EQ(runFn<Int>(L"test.bs.testMaybeInv", 1), 1);

	CHECK_EQ(runFn<Int>(L"test.bs.testMaybeInv2", 0), 10);
	CHECK_EQ(runFn<Int>(L"test.bs.testMaybeInv2", 1), 1);
} END_TEST


/**
 * Constructors.
 */

BEGIN_TEST(StormCtorTest, BS) {
	CHECK_EQ(runFn<Int>(L"test.bs.ctorTest"), 50);
	CHECK_EQ(runFn<Int>(L"test.bs.ctorTest", 10), 30);
	CHECK_EQ(runFn<Int>(L"test.bs.ctorTestDbg", 10), 30);
	CHECK_RUNS(runFn<void>(L"test.bs.ignoreCtor"));
	CHECK_EQ(runFn<Int>(L"test.bs.ctorDerTest", 2), 6);
	CHECK_ERROR(runFn<Int>(L"test.bs.ctorErrorTest"), CodeError);
	CHECK_ERROR(runFn<Int>(L"test.bs.memberAssignErrorTest"), CodeError);
	CHECK_EQ(runFn<Int>(L"test.bs.testDefaultCtor"), 60);
} END_TEST


/**
 * Scoping.
 */

BEGIN_TEST(ScopeTest, BS) {
	CHECK_EQ(runFn<Int>(L"test.bs.testScopeCls"), 10);
	CHECK_EQ(runFn<Int>(L"test.bs.testClassMember"), 20);
	CHECK_EQ(runFn<Int>(L"test.bs.testClassNonmember"), 20);
} END_TEST


/**
 * Units.
 */

BEGIN_TEST(UnitTest, BS) {
	CHECK_EQ(runFn<Duration>(L"test.bs.testUnit"), ms(1) + s(1));
} END_TEST


/**
 * Compile all code and see what happens.
 */

BEGIN_TEST(CompileTest, BS) {
	Engine &e = gEngine();

	TODO(L"Enable me!");
	// Compile all code in core and lang. Not everything in test is supposed to compile cleanly.
	// CHECK_RUNS(e.package(L"core")->compile());
	// CHECK_RUNS(e.package(L"lang")->compile());
} END_TEST

/**
 * Heavy tests.
 */

// Test the REPL of BS programmatically.
BEGIN_TESTX(ReplTest, BS) {
	runFn<void>(L"test.bs.replTest");
} END_TEST


BEGIN_TESTX(BFTest, BS) {
	// Takes a long time to run. Mostly here for testing.
	runFn<Int>(L"test.bf.separateBf");
	runFn<Int>(L"test.bf.inlineBf");
} END_TEST
