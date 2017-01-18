#include "stdafx.h"
#include "Fn.h"
#include "Compiler/Debug.h"

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
	using storm::debug::DbgVal;
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
	// CHECK_EQ(runFn<Int>(L"test.bs.testValVal", 22), 22);
	// CHECK_EQ(runFn<Int>(L"test.bs.testCopyValVal", 22), 22);
	// CHECK_EQ(runFn<Int>(L"test.bs.testAssignValVal", 22), 22);
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