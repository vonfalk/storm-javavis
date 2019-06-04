#include "stdafx.h"
#include "Fn.h"
#include "Compiler/Debug.h"
#include "Compiler/Exception.h"
#include "Compiler/Package.h"
#include "Core/Timing.h"
#include "Core/Set.h"

using storm::debug::DbgVal;
using storm::debug::Dbg;
using storm::debug::DbgActor;

BEGIN_TEST(Priority, BS) {
	CHECK_EQ(runFn<Int>(S("test.bs.prio1")), 203);
	CHECK_EQ(runFn<Int>(S("test.bs.prio2")), 203);
	CHECK_EQ(runFn<Int>(S("test.bs.prio3")), 230);
	CHECK_EQ(runFn<Int>(S("test.bs.prio4")), 230);
	CHECK_EQ(runFn<Int>(S("test.bs.prio5")), 6010);
	CHECK_EQ(runFn<Int>(S("test.bs.prio6")), 11);
	CHECK_EQ(runFn<Int>(S("test.bs.prio7")), 0);
	CHECK_EQ(runFn<Int>(S("test.bs.prio8")), 2);
	CHECK_EQ(runFn<Int>(S("test.bs.prio9")), 2);

	CHECK_EQ(runFn<Int>(S("test.bs.combOp1")), 24);
	CHECK_EQ(runFn<Int>(S("test.bs.combOp2")), 24);
} END_TEST

BEGIN_TEST(Overload, BS) {
	// If this fails, the compiler does not choose the correct overload in the case of ambiguities.
	CHECK_EQ(runFn<Int>(S("test.bs.testOverload"), 1), 1);
	CHECK_EQ(runFn<Int>(S("test.bs.testOverload"), 0), 2);
} END_TEST

BEGIN_TEST(InheritanceTest, BS) {
	using storm::debug::Dbg;

	// Some inheritance testing.
	CHECK_EQ(runFn<Int>(S("test.bs.testBase")), 10);
	CHECK_EQ(runFn<Int>(S("test.bs.testDerived")), 20);

	// Using C++ classes as base.
	CHECK_EQ(runFn<Int>(S("test.bs.testCpp"), 1), 10);
	CHECK_EQ(runFn<Int>(S("test.bs.testCpp"), 2), 20);

	Dbg *created = runFn<Dbg *>(S("test.bs.createCpp"), 1);
	CHECK_EQ(created->get(), 10);
	created = runFn<Dbg *>(S("test.bs.createCpp"), 2);
	CHECK_EQ(created->get(), 20);

	// Use of 'super'.
	CHECK_EQ(runFn<Int>(S("test.bs.testSuperExpr")), 30);

	// Use of 'as'.
	CHECK_EQ(runFn<Int>(S("test.bs.testAsExpr"), 0), 30);
	CHECK_EQ(runFn<Int>(S("test.bs.testAsExpr"), 1), 12);
	CHECK_EQ(runFn<Int>(S("test.bs.testAsExpr2"), 0), 30);
	CHECK_EQ(runFn<Int>(S("test.bs.testAsExpr2"), 1), 12);

	// Inheritance together with visibility. Private members should not interact with other classes.
	CHECK_EQ(runFn<Int>(S("test.bs.testAccess"), 0, 0), 10); // Base, private
	CHECK_EQ(runFn<Int>(S("test.bs.testAccess"), 1, 0), 10); // Derived, private
	CHECK_EQ(runFn<Int>(S("test.bs.testAccess"), 0, 1), 10); // Base, protected
	CHECK_EQ(runFn<Int>(S("test.bs.testAccess"), 1, 1), 20); // Derived, protected

	// These should fail for various reasons.
	CHECK_ERROR(runFn<Int>(S("test.bs.testCallProt")), SyntaxError);
	CHECK_ERROR(runFn<Int>(S("test.bs.testCallPriv")), SyntaxError);
	CHECK_ERROR(runFn<Int>(S("test.bs.testPrivSuper")), SyntaxError);

	// Inner classes should be able to access private things.
	CHECK_EQ(runFn<Int>(S("test.bs.testInner")), 25);
} END_TEST

BEGIN_TEST(AbstractTest, BS) {
	CHECK_EQ(runFn<Int>(S("test.bs.createNoAbstract")), 10);
	CHECK_ERROR(runFn<Int>(S("test.bs.createAbstract")), InstantiationError);
} END_TEST

BEGIN_TEST(FinalTest, BS) {
	CHECK_EQ(runFn<Int>(S("test.bs.createFinalBase")), 10);
	// Overriding a final function.
	CHECK_ERROR(runFn<Int>(S("test.bs.createFinalA")), TypedefError);
	// Using 'override' properly.
	CHECK_EQ(runFn<Int>(S("test.bs.createFinalB")), 30);
	// Not overriding, even though 'override' is specified.
	CHECK_ERROR(runFn<Int>(S("test.bs.createFinalC")), TypedefError);
	// Overrides, but with invalid return type.
	CHECK_ERROR(runFn<Int>(S("test.bs.createFinalD")), TypedefError);
} END_TEST

BEGIN_TEST(StaticTest, BS) {
	CHECK_EQ(runFn<Int>(S("test.bs.testStatic")), 52);
	CHECK_EQ(runFn<Int>(S("test.bs.testStaticInheritance"), 0), 10);
	CHECK_EQ(runFn<Int>(S("test.bs.testStaticInheritance"), 1), 10);
} END_TEST

/**
 * Values.
 */

BEGIN_TEST(ValueTest, BS) {
	// Values.
	DbgVal::clear();
	CHECK_EQ(runFn<Int>(S("test.bs.testValue")), 10);
	CHECK(DbgVal::clear());
	CHECK_EQ(runFn<Int>(S("test.bs.testDefInit")), 10);
	CHECK(DbgVal::clear());
	CHECK_EQ(runFn<Int>(S("test.bs.testValAssign")), 20);
	CHECK(DbgVal::clear());
	CHECK_EQ(runFn<Int>(S("test.bs.testValCopy")), 20);
	CHECK(DbgVal::clear());
	CHECK_EQ(runFn<Int>(S("test.bs.testValCtor")), 7);
	CHECK(DbgVal::clear());
	CHECK_EQ(runFn<Int>(S("test.bs.testValParam")), 16);
	CHECK(DbgVal::clear());
	CHECK_EQ(runFn<Int>(S("test.bs.testValReturn")), 22);
	CHECK(DbgVal::clear());
	CHECK_EQ(runFn<DbgVal>(S("test.bs.createVal"), 20), DbgVal(20));
	CHECK(DbgVal::clear());
	CHECK_EQ((runFn<Int, DbgVal>(S("test.bs.asVal"), DbgVal(11))), 13);
	CHECK(DbgVal::clear());
	// TODO: See if we can test so that destructors are executed from within classes/actors.
} END_TEST

BEGIN_TEST(CustomValueTest, BS) {
	CHECK_EQ(runFn<Int>(S("test.bs.testCustomValue")), -300);
	CHECK_EQ(runFn<Int>(S("test.bs.testRefVal"), 24), 24);
	CHECK_EQ(runFn<Int>(S("test.bs.testCopyRefVal"), 24), 24);
	CHECK_EQ(runFn<Int>(S("test.bs.testAssignRefVal"), 24), 24);
	CHECK_EQ(runFn<Int>(S("test.bs.testValVal"), 22), 22);
	CHECK_EQ(runFn<Int>(S("test.bs.testCopyValVal"), 22), 22);
	CHECK_EQ(runFn<Int>(S("test.bs.testAssignValVal"), 22), 22);
} END_TEST

BEGIN_TEST(ValueMemberTest, BS) {
	CHECK_EQ(runFn<Int>(S("test.bs.testVirtualVal1")), 10);
	CHECK_EQ(runFn<Int>(S("test.bs.testVirtualVal2")), 20);
	CHECK_EQ(runFn<Int>(S("test.bs.testVirtualVal3")), 15);
	// This test is really good in release builts. For VS2008, the compiler uses
	// the return value (in eax) of v->asDbgVal(), and crashes if we fail to return
	// a correct value. In debug builds, the compiler usually re-loads the value
	// itself instead.
	Dbg *v = runFn<Dbg *>(S("test.bs.testVirtualVal4"));
	CHECK_EQ(v->asDbgVal().v, 20);

	// Does the thread thunks correctly account for the special handling of member functions?
	CHECK_EQ(runFn<Int>(S("test.bs.testActorVal")), 10);
} END_TEST


/**
 * Autocast.
 */

BEGIN_TEST(AutocastTest, BS) {
	// Check auto-casting from int to nat.
	CHECK_EQ(runFn<Int>(S("test.bs.castToNat")), 20);
	CHECK_EQ(runFn<Int>(S("test.bs.castToMaybe")), 20);
	CHECK_EQ(runFn<Int>(S("test.bs.downcastMaybe")), 20);
	CHECK_RUNS(runFn<void>(S("test.bs.ifCast")));
	CHECK_EQ(runFn<Int>(S("test.bs.autoCast"), 5), 10);
	CHECK_EQ(runFn<Float>(S("test.bs.promoteCtor")), 2);
	CHECK_EQ(runFn<Float>(S("test.bs.promoteInit")), 8);
	CHECK_EQ(runFn<Nat>(S("test.bs.initNat")), 20);
	CHECK_EQ(runFn<Float>(S("test.bs.opFloat")), 5.0f);
} END_TEST


/**
 * Automatic generation of operators.
 */

BEGIN_TEST(OperatorTest, BS) {
	CHECK_EQ(runFn<Bool>(S("test.bs.opLT"), 10, 20), true);
	CHECK_EQ(runFn<Bool>(S("test.bs.opLT"), 20, 10), false);
	CHECK_EQ(runFn<Bool>(S("test.bs.opLT"), 10, 10), false);
	CHECK_EQ(runFn<Bool>(S("test.bs.opGT"), 10, 20), false);
	CHECK_EQ(runFn<Bool>(S("test.bs.opGT"), 20, 10), true);
	CHECK_EQ(runFn<Bool>(S("test.bs.opGT"), 10, 10), false);
	CHECK_EQ(runFn<Bool>(S("test.bs.opLTE"), 10, 20), true);
	CHECK_EQ(runFn<Bool>(S("test.bs.opLTE"), 20, 10), false);
	CHECK_EQ(runFn<Bool>(S("test.bs.opLTE"), 20, 20), true);
	CHECK_EQ(runFn<Bool>(S("test.bs.opGTE"), 10, 20), false);
	CHECK_EQ(runFn<Bool>(S("test.bs.opGTE"), 20, 10), true);
	CHECK_EQ(runFn<Bool>(S("test.bs.opGTE"), 10, 10), true);
	CHECK_EQ(runFn<Bool>(S("test.bs.opEQ"), 10, 20), false);
	CHECK_EQ(runFn<Bool>(S("test.bs.opEQ"), 20, 10), false);
	CHECK_EQ(runFn<Bool>(S("test.bs.opEQ"), 10, 10), true);
	CHECK_EQ(runFn<Bool>(S("test.bs.opNEQ"), 10, 20), true);
	CHECK_EQ(runFn<Bool>(S("test.bs.opNEQ"), 20, 10), true);
	CHECK_EQ(runFn<Bool>(S("test.bs.opNEQ"), 10, 10), false);

	// Check the == operator for actors.
	DbgActor *a = new (gEngine()) DbgActor(1);
	DbgActor *b = new (gEngine()) DbgActor(2);

	CHECK_EQ(runFn<Bool>(S("test.bs.opActorEQ"), a, b), false);
	CHECK_EQ(runFn<Bool>(S("test.bs.opActorEQ"), a, a), true);
	CHECK_EQ(runFn<Bool>(S("test.bs.opActorNEQ"), a, b), true);
	CHECK_EQ(runFn<Bool>(S("test.bs.opActorNEQ"), a, a), false);
} END_TEST

BEGIN_TEST(SetterTest, BS) {
	// The hard part with this test is not getting the correct result, it is getting it to compile!
	CHECK_EQ(runFn<Int>(S("test.bs.testSetter")), 20);
	CHECK_EQ(runFn<Int>(S("test.bs.testSetterPair")), 20);
	CHECK_EQ(runFn<Float>(S("test.bs.testSetterCpp")), 50.0f);

	// When trying to use anything else as a setter, a syntax error should be thrown!
	CHECK_ERROR(runFn<Int>(S("test.bs.testNoSetter")), SyntaxError);
} END_TEST


/**
 * Type system.
 */

BEGIN_TEST(TypesTest, BS) {
	CHECK_ERROR(runFn<void>(S("test.bs.invalidDecl")), SyntaxError);
	CHECK_ERROR(runFn<void>(S("test.bs.voidExpr")), SyntaxError);
} END_TEST


/**
 * Maybe.
 */

BEGIN_TEST(MaybeTest, BS) {
	CHECK_EQ(runFn<Int>(S("test.bs.testMaybe"), 0), 0);
	CHECK_EQ(runFn<Int>(S("test.bs.testMaybe"), 1), 2);
	CHECK_EQ(runFn<Int>(S("test.bs.testMaybe"), 2), 6);

	CHECK_EQ(runFn<Int>(S("test.bs.assignMaybe")), 1);
	CHECK_ERROR(runFn<void>(S("test.bs.assignError")), SyntaxError);

	CHECK_EQ(::toS(runFn<Str *>(S("test.bs.maybeToS"), 0)), L"null");
	CHECK_EQ(::toS(runFn<Str *>(S("test.bs.maybeToS"), 1)), L"ok");

	CHECK_EQ(runFn<Int>(S("test.bs.maybeInheritance")), 10);

	CHECK_EQ(runFn<Int>(S("test.bs.testMaybeInv"), 0), 10);
	CHECK_EQ(runFn<Int>(S("test.bs.testMaybeInv"), 1), 1);

	CHECK_EQ(runFn<Int>(S("test.bs.testMaybeInv2"), 0), 10);
	CHECK_EQ(runFn<Int>(S("test.bs.testMaybeInv2"), 1), 1);

	// Values!
	CHECK_EQ(runFn<Int>(S("test.bs.testMaybeValue"), 0), 8);
	CHECK_EQ(runFn<Int>(S("test.bs.testMaybeValue"), 1), 1);

	CHECK_EQ(runFn<Int>(S("test.bs.testMaybeValueAny"), 0), 0);
	CHECK_EQ(runFn<Int>(S("test.bs.testMaybeValueAny"), 1), 1);

	CHECK_EQ(::toS(runFn<Str *>(S("test.bs.testMaybeValueToS"), 0)), L"null");
	CHECK_EQ(::toS(runFn<Str *>(S("test.bs.testMaybeValueToS"), 5)), L"5");
} END_TEST


/**
 * Constructors.
 */

BEGIN_TEST(StormCtorTest, BS) {
	CHECK_EQ(runFn<Int>(S("test.bs.ctorTest")), 50);
	CHECK_EQ(runFn<Int>(S("test.bs.ctorTest"), 10), 30);
	CHECK_EQ(runFn<Int>(S("test.bs.ctorTestDbg"), 10), 30);
	CHECK_RUNS(runFn<void>(S("test.bs.ignoreCtor")));
	CHECK_EQ(runFn<Int>(S("test.bs.ctorDerTest"), 2), 6);
	CHECK_ERROR(runFn<Int>(S("test.bs.ctorErrorTest")), CodeError);
	CHECK_ERROR(runFn<Int>(S("test.bs.memberAssignErrorTest")), CodeError);
	CHECK_EQ(runFn<Int>(S("test.bs.testDefaultCtor")), 60);
	CHECK_EQ(runFn<Int>(S("test.bs.testImplicitInit")), 50);

	// Initialization order.
	CHECK_EQ(runFn<Int>(S("test.bs.checkInitOrder")), 321);
	CHECK_EQ(runFn<Int>(S("test.bs.checkInitOrder2")), 123);

	// Two-step initialization.
	CHECK_RUNS(runFn<void>(S("test.bs.twoStepInit")));
	CHECK_ERROR(runFn<void>(S("test.bs.twoStepFail")), SyntaxError);
} END_TEST


/**
 * Scoping.
 */

BEGIN_TEST(ScopeTest, BS) {
	CHECK_EQ(runFn<Int>(S("test.bs.testScopeCls")), 10);
	CHECK_EQ(runFn<Int>(S("test.bs.testClassMember")), 20);
	CHECK_EQ(runFn<Int>(S("test.bs.testClassNonmember")), 20);
} END_TEST


/**
 * Units.
 */

BEGIN_TEST(UnitTest, BS) {
	CHECK_EQ(runFn<Duration>(S("test.bs.testUnit")), time::ms(1) + time::s(1));
} END_TEST


/**
 * Return.
 */

BEGIN_TEST(ReturnTest, BS) {
	// Return integers.
	CHECK_EQ(runFn<Int>(S("test.bs.returnInt"), 10), 10);
	CHECK_EQ(runFn<Int>(S("test.bs.returnInt"), 40), 20);

	// Return strings.
	CHECK_EQ(::toS(runFn<Str *>(S("test.bs.returnStr"), 10)), L"no");
	CHECK_EQ(::toS(runFn<Str *>(S("test.bs.returnStr"), 40)), L"40");

	// Return values.
	DbgVal::clear();
	CHECK_EQ(runFn<DbgVal>(S("test.bs.returnDbgVal"), 11).get(), 10);
	CHECK_EQ(runFn<DbgVal>(S("test.bs.returnDbgVal"), 30).get(), 20);
	CHECK(DbgVal::clear());

	// Return type checking, interaction with 'no-return' returns.
	CHECK_EQ(runFn<Int>(S("test.bs.returnAlways"), 22), 22);
	CHECK_EQ(runFn<Int>(S("test.bs.deduceType"), 21), 22);
	CHECK_EQ(runFn<Int>(S("test.bs.prematureReturn"), 20), 30);
} END_TEST


/**
 * Arrays.
 */

BEGIN_TEST(BSArrayTest, BS) {
	CHECK_EQ(runFn<Int>(S("test.bs.testArray")), 230);
	CHECK_EQ(runFn<Int>(S("test.bs.testIntArray")), 95);
	CHECK_EQ(runFn<Int>(S("test.bs.testInitArray")), 1337);
	CHECK_EQ(runFn<Int>(S("test.bs.testInitAutoArray")), 1234);
	CHECK_EQ(runFn<Int>(S("test.bs.testAutoArray")), 0);
	CHECK_EQ(runFn<Int>(S("test.bs.testCastArray")), 2);
	CHECK_EQ(runFn<Int>(S("test.bs.testIterator")), 15);
	CHECK_EQ(runFn<Int>(S("test.bs.testIteratorIndex")), 16);

	// Interoperability.
	Array<Int> *r = runFn<Array<Int> *>(S("test.bs.createValArray"));
	CHECK_EQ(r->count(), 20);
	for (nat i = 0; i < 20; i++) {
		CHECK_EQ(r->at(i), i);
	}

	// Sorting.
	CHECK_EQ(toS(runFn<Str *>(S("test.bs.sortArray"))), L"[1, 2, 3, 4, 5]");
	CHECK_EQ(toS(runFn<Str *>(S("test.bs.sortedArray"))), L"[1, 2, 3, 4, 5][5, 4, 3, 2, 1]");
	CHECK_EQ(toS(runFn<Str *>(S("test.bs.sortArrayP"))), L"[3, 4, 5, 1, 2]");
	CHECK_EQ(toS(runFn<Str *>(S("test.bs.sortedArrayP"))), L"[3, 4, 5, 1, 2][5, 4, 3, 2, 1]");
} END_TEST

BEGIN_TEST(BSPQTest, BS) {
	CHECK_EQ(runFn<Int>(S("test.bs.pqSecond")), 8);
	CHECK_EQ(runFn<Int>(S("test.bs.pqInit")), 8);
	CHECK_EQ(runFn<Int>(S("test.bs.pqCompare")), 5);
	CHECK_EQ(runFn<Int>(S("test.bs.pqCompareInit")), 2);
	CHECK_EQ(toS(runFn<Str *>(S("test.bs.pqStr"))), L"World");
} END_TEST

/**
 * Map.
 */

BEGIN_TEST(BSMapTest, BS) {
	Engine &e = gEngine();
	{
		Array<Int> *keys = new (e) Array<Int>();
		Array<Int> *vals = new (e) Array<Int>();

		keys->push(10);
		keys->push(100);
		vals->push(5);
		vals->push(8);

		Map<Int, Int> *map = runFn<Map<Int, Int> *>(S("test.bs.intMapTest"), keys, vals);
		CHECK_EQ(map->count(), 2);
		CHECK_EQ(map->get(10), 5);
		CHECK_EQ(map->get(100), 8);

		CHECK_EQ(runFn<Int>(S("test.bs.iterateMap"), map), 850);
	}

	{
		Array<Str *> *keys = new (e) Array<Str *>();
		Array<Str *> *vals = new (e) Array<Str *>();

		keys->push(new (e) Str(S("A")));
		keys->push(new (e) Str(S("B")));
		vals->push(new (e) Str(S("80")));
		vals->push(new (e) Str(S("90")));

		Map<Str *, Str *> *map = runFn<Map<Str *, Str *> *>(S("test.bs.strMapTest"), keys, vals);
		CHECK_EQ(map->count(), 2);
		CHECK_EQ(toS(runFn<Str *>(S("test.bs.readStrMap"), map, new (e) Str(S("A")))), S("80"));
		CHECK_EQ(toS(runFn<Str *>(S("test.bs.readStrMap"), map, new (e) Str(S("B")))), S("90"));
	}

} END_TEST


/**
 * Set.
 */

BEGIN_TEST(BSSetTest, BS) {
	Set<Int> *s = new (gEngine()) Set<Int>();
	s->put(100);
	s->put(80);
	s->put(9);

	CHECK_EQ(runFn<Int>(S("test.bs.iterateSet"), s), 189);
} END_TEST

BEGIN_TEST(BSWeakSetTest, BS) {
	Array<DbgActor *> *a = new (gEngine()) Array<DbgActor *>();
	a->push(new (gEngine()) DbgActor(10));
	a->push(new (gEngine()) DbgActor(80));
	a->push(new (gEngine()) DbgActor(200));

	WeakSet<DbgActor> *s = new (gEngine()) WeakSet<DbgActor>();
	for (Nat i = 0; i < a->count(); i++)
		s->put(a->at(i));

	CHECK_EQ(runFn<Int>(S("test.bs.iterateWeakSetPlain"), s), 290);
	CHECK_EQ(runFn<Int>(S("test.bs.iterateWeakSet"), s), 290);
} END_TEST


/**
 * Enums.
 */

BEGIN_TEST(BSEnumTest, BS) {
	CHECK_EQ(::toS(runFn<Str *>(S("test.bs.enum1"))), L"foo");
	CHECK_EQ(::toS(runFn<Str *>(S("test.bs.enumBit1"))), L"bitFoo + bitBar");
} END_TEST


/**
 * Clone.
 */

BEGIN_TEST(CloneTest, BS) {
	CHECK(runFn<Bool>(S("test.bs.testClone")));
	CHECK(runFn<Bool>(S("test.bs.testCloneDerived")));
	CHECK(runFn<Bool>(S("test.bs.testCloneValue")));
	CHECK_EQ(runFn<Int>(S("test.bs.testCloneArray")), 10);
} END_TEST


/**
 * Generate.
 */

BEGIN_TEST(GenerateTest, BS) {
	CHECK_EQ(runFn<Int>(S("test.bs.genAdd"), 10, 20), 30);
	CHECK_EQ(runFn<Float>(S("test.bs.genAdd"), 10.2f, 20.3f), 30.5f);
	CHECK_EQ(runFn<Int>(S("test.bs.testGenClass"), 10), 12);
} END_TEST


/**
 * Patterns.
 */

BEGIN_TEST(PatternTest, BS) {
	CHECK_EQ(runFn<Int>(S("test.bs.testPattern")), 170);
	CHECK_EQ(runFn<Int>(S("test.bs.testPatternNames")), 2010);

	CHECK_EQ(runFn<Int>(S("test.bs.testPatternSplice1")), 23);
	CHECK_EQ(runFn<Int>(S("test.bs.testPatternSplice2")), 6);

	// This is enforced by the syntax, but also by an explicit check in the code.
	CHECK_ERROR(runFn<void>(S("test.bs.errors.testPatternOutside")), SyntaxError);
} END_TEST

/**
 * Lambda functions.
 */

BEGIN_TEST(LambdaTest, BS) {
	CHECK_EQ(runFn<Int>(S("test.bs.testLambda")), 13);
	CHECK_EQ(runFn<Int>(S("test.bs.testLambdaParam")), 15);
	CHECK_EQ(runFn<Int>(S("test.bs.testLambdaVar")), 32);
	CHECK_EQ(runFn<Int>(S("test.bs.testLambdaCapture")), 30);
	CHECK_EQ(runFn<Int>(S("test.bs.testLambdaCapture2")), 53);
	CHECK_EQ(toS(runFn<Str *>(S("test.bs.testLambdaMemory"))), L"[0, 1, 2, 0, 1, 3]");
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
	CALL_TEST_FN(checkTimes, S("test.bs.basicException"), 7);
	CALL_TEST_FN(checkTimes, S("test.bs.fnException"), 3);
	CALL_TEST_FN(checkTimes, S("test.bs.ctorError"), 8);
	CALL_TEST_FN(checkTimes, S("test.bs.threadException"), 4);
} END_TEST

// Global variables.
BEGIN_TEST(Globals, BS) {
	Engine &e = gEngine();

	CHECK_EQ(runFn<Int>(S("test.bs.testGlobal"), 10), 0);
	CHECK_EQ(runFn<Int>(S("test.bs.testGlobal"), 5), 10);
	CHECK_EQ(runFn<Int>(S("test.bs.testGlobal"), 7), 5);

	Str *strA = new (e) Str(S("A"));
	Str *strB = new (e) Str(S("B"));
	CHECK_EQ(toS(runFn<Str *>(S("test.bs.testGlobal"), strA)), L"Hello");
	CHECK_EQ(runFn<Str *>(S("test.bs.testGlobal"), strB), strA);
	CHECK_EQ(runFn<Str *>(S("test.bs.testGlobal"), strA), strB);

	debug::DbgNoToS val;
	val.dummy = 1;
	CHECK_EQ(runFn<debug::DbgNoToS>(S("test.bs.testGlobal"), val).dummy, 0);
	CHECK_EQ(runFn<debug::DbgNoToS>(S("test.bs.testGlobal"), val).dummy, 1);

	// From other threads (would be good to check initialization is properly done as well).
	CHECK_EQ(toS(runFn<Str *>(S("test.bs.threadGlobal"))), L"Other");

	CHECK_ERROR(runFn<Str *>(S("test.bs.failThreadGlobal")), SyntaxError);

	// Make sure we have the proper initialization order.
	CHECK_EQ(toS(runFn<Str *>(S("test.bs.getInitGlobal"))), L"Global: A");

} END_TEST

/**
 * Heavy tests.
 */

// Test the REPL of BS programmatically.
BEGIN_TESTX(ReplTest, BS) {
	runFn<void>(S("test.bs.replTest"));
} END_TEST


BEGIN_TESTX(BFTest, BS) {
	// Takes a long time to run. Mostly here for testing.
	CHECK_RUNS(runFn<void>(S("test.bf.separateBf")));
	CHECK_RUNS(runFn<void>(S("test.bf.inlineBf")));
} END_TEST
