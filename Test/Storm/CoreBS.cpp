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
	CHECK_EQ(runFn<Int>(L"test.bs.testActorVal"), 10);
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
 * Type system.
 */

BEGIN_TEST(TypesTest, BS) {
	CHECK_ERROR(runFn<void>(L"test.bs.invalidDecl"), SyntaxError);
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
	CHECK_EQ(runFn<Duration>(L"test.bs.testUnit"), time::ms(1) + time::s(1));
} END_TEST


/**
 * Compile all code and see what happens.
 */

BEGIN_TEST(CompileTest, BS) {
	Engine &e = gEngine();

	// Compile all code in core and lang. Not everything in test is supposed to compile cleanly.
	CHECK_RUNS(e.package(L"core")->compile());
	CHECK_RUNS(e.package(L"lang")->compile());
} END_TEST


/**
 * Return.
 */

BEGIN_TEST(ReturnTest, BS) {
	// Return integers.
	CHECK_EQ(runFn<Int>(L"test.bs.returnInt", 10), 10);
	CHECK_EQ(runFn<Int>(L"test.bs.returnInt", 40), 20);

	// Return strings.
	CHECK_EQ(::toS(runFn<Str *>(L"test.bs.returnStr", 10)), L"no");
	CHECK_EQ(::toS(runFn<Str *>(L"test.bs.returnStr", 40)), L"40");

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


/**
 * Arrays.
 */

BEGIN_TEST(BSArrayTest, BS) {
	CHECK_EQ(runFn<Int>(L"test.bs.testArray"), 230);
	CHECK_EQ(runFn<Int>(L"test.bs.testIntArray"), 95);
	CHECK_EQ(runFn<Int>(L"test.bs.testInitArray"), 1337);
	CHECK_EQ(runFn<Int>(L"test.bs.testInitAutoArray"), 1234);
	CHECK_EQ(runFn<Int>(L"test.bs.testAutoArray"), 0);
	CHECK_EQ(runFn<Int>(L"test.bs.testCastArray"), 2);
	CHECK_EQ(runFn<Int>(L"test.bs.testIterator"), 15);
	CHECK_EQ(runFn<Int>(L"test.bs.testIteratorIndex"), 16);

	// Interoperability.
	Array<Int> *r = runFn<Array<Int> *>(L"test.bs.createValArray");
	CHECK_EQ(r->count(), 20);
	for (nat i = 0; i < 20; i++) {
		CHECK_EQ(r->at(i), i);
	}
} END_TEST


/**
 * Enums.
 */

BEGIN_TEST(BSEnumTest, BS) {
	CHECK_EQ(::toS(runFn<Str *>(L"test.bs.enum1")), L"foo");
	CHECK_EQ(::toS(runFn<Str *>(L"test.bs.enumBit1")), L"bitFoo + bitBar");
} END_TEST


/**
 * Clone.
 */

BEGIN_TEST(CloneTest, BS) {
	CHECK(runFn<Bool>(L"test.bs.testClone"));
	CHECK(runFn<Bool>(L"test.bs.testCloneDerived"));
	CHECK(runFn<Bool>(L"test.bs.testCloneValue"));
	CHECK_EQ(runFn<Int>(L"test.bs.testCloneArray"), 10);
} END_TEST


/**
 * Generate.
 */

BEGIN_TEST(GenerateTest, BS) {
	CHECK_EQ(runFn<Int>(L"test.bs.genAdd", 10, 20), 30);
	CHECK_EQ(runFn<Float>(L"test.bs.genAdd", 10.2f, 20.3f), 30.5f);
	CHECK_EQ(runFn<Int>(L"test.bs.testGenClass", 10), 12);
} END_TEST


/**
 * Exception safety.
 */

BEGIN_TEST_FN(checkTimes, const wchar *name, nat times) {
	DbgVal::clear();
	CHECK_RUNS(runFn<void>(name, Int(0)));
	CHECK(DbgVal::clear());
	for (nat i = 0; i < times; i++) {
		CHECK_ERROR(runFn<void>(name, Int(i + 1)), DebugError);
		CHECK(DbgVal::clear());
	}
	CHECK_RUNS(runFn<void>(name, Int(times + 1)));
	CHECK(DbgVal::clear());
} END_TEST_FN

// Tests that checks the exception safety at various times in the generated code. Especially
// with regards to values.
BEGIN_TEST(BSException, BS) {
	CALL_TEST_FN(checkTimes, L"test.bs.basicException", 7);
	CALL_TEST_FN(checkTimes, L"test.bs.fnException", 3);
	CALL_TEST_FN(checkTimes, L"test.bs.threadException", 4);
	CALL_TEST_FN(checkTimes, L"test.bs.ctorError", 8);
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
	CHECK_RUNS(runFn<void>(L"test.bf.separateBf"));
	CHECK_RUNS(runFn<void>(L"test.bf.inlineBf"));
} END_TEST
