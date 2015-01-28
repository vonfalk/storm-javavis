#include "stdafx.h"
#include "Test/Test.h"
#include "Storm/Function.h"
#include "Storm/Lib/Debug.h"
#include "Code/Function.h"

Int runFn(const String &fn) {
	Engine &e = *gEngine;
	Function *fun = as<Function>(e.scope()->find(Name(fn), vector<Value>()));
	if (!fun)
		throw TestError(L"Function " + fn + L" was not found.");
	void *ptr = fun->pointer();
	if (!ptr)
		throw TestError(L"Function " + fn + L" did not return any code.");
	return code::FnCall().call<Int>(ptr);
}

Int runFn(const String &fn, Int p) {
	Engine &e = *gEngine;
	Function *fun = as<Function>(e.scope()->find(Name(fn), vector<Value>(1, Value(intType(e)))));
	if (!fun)
		throw TestError(L"Function " + fn + L" was not found.");
	void *ptr = fun->pointer();
	if (!ptr)
		throw TestError(L"Function " + fn + L" did not return any code.");
	return code::FnCall().param(p).call<Int>(ptr);
}

Object *runObjFn(const String &fn, Int p) {
	Engine &e = *gEngine;
	Function *fun = as<Function>(e.scope()->find(Name(fn), vector<Value>(1, Value(intType(e)))));
	if (!fun)
		throw TestError(L"Function " + fn + L" was not found.");
	void *ptr = fun->pointer();
	if (!ptr)
		throw TestError(L"Function " + fn + L" did not return any code.");
	return code::FnCall().param(p).call<Object *>(ptr);
}


BEGIN_TEST(LoadCode) {
	Engine &engine = *gEngine;
	Package *test = engine.package(Name(L"test.bs"));

	CHECK(test->find(Name(L"bar")));
	CHECK(test->find(Name(L"Foo")));
	CHECK(test->find(Name(L"Foo.a")));
	CHECK(test->find(Name(L"Foo.foo")));
} END_TEST

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

	CHECK_ERROR(runFn(L"test.bs.assignError"));
} END_TEST

BEGIN_TEST(InheritanceTest) {
	// Some inheritance testing.
	CHECK_EQ(runFn(L"test.bs.testBase"), 10);
	CHECK_EQ(runFn(L"test.bs.testDerived"), 20);

	// Using C++ classes as base.
	CHECK_EQ(runFn(L"test.bs.testCpp", 1), 10);
	CHECK_EQ(runFn(L"test.bs.testCpp", 2), 20);

	Auto<Object> created = runObjFn(L"test.bs.createCpp", 1);
	CHECK_EQ(created.expect<Dbg>(*gEngine, L"dbg")->get(), 10);
	created = runObjFn(L"test.bs.createCpp", 2);
	CHECK_EQ(created.expect<Dbg>(*gEngine, L"dbg")->get(), 20);
} END_TEST

BEGIN_TEST(ValueTest) {
	// Values.
	CHECK_EQ(runFn(L"test.bs.testValue"), 10);
} END_TEST
