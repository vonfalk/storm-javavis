#include "stdafx.h"
#include "Test/Test.h"
#include "Storm/Function.h"
#include "Storm/Lib/Debug.h"
#include "Code/Function.h"

template <class T>
T runFn(const String &fn) {
	Engine &e = *gEngine;
	Auto<Name> fName = parseSimpleName(e, fn);
	Function *fun = as<Function>(e.scope()->find(fName));
	if (!fun)
		throw TestError(L"Function " + fn + L" was not found.");
	void *ptr = fun->pointer();
	if (!ptr)
		throw TestError(L"Function " + fn + L" did not return any code.");
	return code::FnCall().call<T>(ptr);
}

template <class T>
T runFn(const String &fn, Int p) {
	Engine &e = *gEngine;
	Auto<Name> fName = parseSimpleName(e, fn);
	fName = fName->withParams(vector<Value>(1, Value(intType(e))));
	Function *fun = as<Function>(e.scope()->find(fName));
	if (!fun)
		throw TestError(L"Function " + fn + L" was not found.");
	void *ptr = fun->pointer();
	if (!ptr)
		throw TestError(L"Function " + fn + L" did not return any code.");
	return code::FnCall().param(p).call<T>(ptr);
}

template <class T, class Par>
T runFn(const String &fn, const Par &par) {
	Engine &e = *gEngine;
	Auto<Name> fName = parseSimpleName(e, fn);
	fName = fName->withParams(vector<Value>(1, Par::type(e)));
	Function *fun = as<Function>(e.scope()->find(fName));
	if (!fun)
		throw TestError(L"Function " + fn + L" was not found.");
	void *ptr = fun->pointer();
	if (!ptr)
		throw TestError(L"Function " + fn + L" did not return any code.");

	return code::FnCall().param(par).call<T>(ptr);
}

Int runFn(const String &fn) {
	return runFn<Int>(fn);
}

Int runFn(const String &fn, Int p) {
	return runFn<Int>(fn, p);
}


BEGIN_TEST(BasicSyntax) {
	CHECK_RUNS(runFn(L"test.bs.voidFn"));

	CHECK_EQ(runFn(L"test.bs.bar"), 3);
	CHECK_EQ(runFn(L"test.bs.ifTest", 1), 3);
	CHECK_EQ(runFn(L"test.bs.ifTest", 2), 4);
	CHECK_EQ(runFn(L"test.bs.ifTest", 3), 5);
	CHECK_EQ(runFn(L"test.bs.ifTest2", 3), 4);
	CHECK_EQ(runFn(L"test.bs.ifTest2", 0), -1);

	CHECK_EQ(runFn(L"test.bs.assign", 1), 2);
	CHECK_EQ(runFn(L"test.bs.while", 10), 1024);
	CHECK_EQ(runFn(L"test.bs.for", 10), 1024);

	CHECK_EQ(runFn(L"test.bs.createFoo"), 3);

	CHECK_EQ(runFn(L"test.bs.testCtor"), 20);
	CHECK_EQ(runFn(L"test.bs.testIntCtor"), 20);
} END_TEST

BEGIN_TEST(InheritanceTest) {
	// Some inheritance testing.
	CHECK_EQ(runFn(L"test.bs.testBase"), 10);
	CHECK_EQ(runFn(L"test.bs.testDerived"), 20);

	// Using C++ classes as base.
	CHECK_EQ(runFn(L"test.bs.testCpp", 1), 10);
	CHECK_EQ(runFn(L"test.bs.testCpp", 2), 20);

	Auto<Object> created = runFn<Object*>(L"test.bs.createCpp", 1);
	CHECK_EQ(created.expect<Dbg>(*gEngine, L"dbg")->get(), 10);
	created = runFn<Object*>(L"test.bs.createCpp", 2);
	CHECK_EQ(created.expect<Dbg>(*gEngine, L"dbg")->get(), 20);
} END_TEST

BEGIN_TEST(ValueTest) {
	// Values.
	DbgVal::clear();
	CHECK_EQ(runFn(L"test.bs.testValue"), 10);
	CHECK(DbgVal::clear());
	CHECK_EQ(runFn(L"test.bs.testDefInit"), 10);
	CHECK(DbgVal::clear());
	CHECK_EQ(runFn(L"test.bs.testValAssign"), 20);
	CHECK(DbgVal::clear());
	CHECK_EQ(runFn(L"test.bs.testValCopy"), 20);
	CHECK(DbgVal::clear());
	CHECK_EQ(runFn(L"test.bs.testValCtor"), 7);
	CHECK(DbgVal::clear());
	CHECK_EQ(runFn(L"test.bs.testValParam"), 16);
	CHECK(DbgVal::clear());
	CHECK_EQ(runFn(L"test.bs.testValReturn"), 22);
	CHECK(DbgVal::clear());
	CHECK_EQ(runFn<DbgVal>(L"test.bs.createVal", 20), DbgVal(20));
	CHECK(DbgVal::clear());
	CHECK_EQ((runFn<Int, DbgVal>(L"test.bs.asVal", DbgVal(11))), 13);
	CHECK(DbgVal::clear());
	CHECK_EQ(runFn(L"test.bs.testContainVal"), 10);
	CHECK(DbgVal::clear());
	CHECK_EQ(runFn(L"test.bs.testContainVal"), 10);
	CHECK(DbgVal::clear());
} END_TEST

BEGIN_TEST(CustomValueTest) {
	CHECK_EQ(runFn(L"test.bs.testCustomValue"), -300);
	CHECK_EQ(runFn(L"test.bs.testRefVal", 24), 24);
	CHECK_EQ(runFn(L"test.bs.testCopyRefVal", 24), 24);
	CHECK_EQ(runFn(L"test.bs.testAssignRefVal", 24), 24);
	CHECK_EQ(runFn(L"test.bs.testValVal", 22), 22);
	CHECK_EQ(runFn(L"test.bs.testCopyValVal", 22), 22);
	CHECK_EQ(runFn(L"test.bs.testAssignValVal", 22), 22);
} END_TEST

BEGIN_TEST(StormArrayTest) {
	CHECK_EQ(runFn(L"test.bs.testArray"), 230);
	CHECK_EQ(runFn(L"test.bs.testValArray"), 250);
	CHECK_EQ(runFn(L"test.bs.testIntArray"), 95);

	// Interoperability.
	Auto<Array<DbgVal>> r = runFn<Array<DbgVal>*>(L"test.bs.createValArray");
	CHECK_EQ(r->count(), 20);
	for (nat i = 0; i < 20; i++) {
		CHECK_EQ(r->at(i).get(), i);
	}

} END_TEST
