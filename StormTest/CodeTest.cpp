#include "stdafx.h"
#include "Test/Test.h"
#include "Storm/Function.h"
#include "Storm/Lib/Debug.h"
#include "Code/Function.h"

Int runFn(Engine &e, const String &fn) {
	Function *fun = as<Function>(e.scope()->find(Name(fn), vector<Value>()));
	if (!fun)
		throw TestError(L"Function " + fn + L" was not found.");
	void *ptr = fun->pointer();
	if (!ptr)
		throw TestError(L"Function " + fn + L" did not return any code.");
	return code::FnCall().call<Int>(ptr);
}

Int runFn(Engine &e, const String &fn, Int p) {
	Function *fun = as<Function>(e.scope()->find(Name(fn), vector<Value>(1, Value(intType(e)))));
	if (!fun)
		throw TestError(L"Function " + fn + L" was not found.");
	void *ptr = fun->pointer();
	if (!ptr)
		throw TestError(L"Function " + fn + L" did not return any code.");
	return code::FnCall().param(p).call<Int>(ptr);
}

Object *runObjFn(Engine &e, const String &fn, Int p) {
	Function *fun = as<Function>(e.scope()->find(Name(fn), vector<Value>(1, Value(intType(e)))));
	if (!fun)
		throw TestError(L"Function " + fn + L" was not found.");
	void *ptr = fun->pointer();
	if (!ptr)
		throw TestError(L"Function " + fn + L" did not return any code.");
	return code::FnCall().param(p).call<Object *>(ptr);
}


BEGIN_TEST(CodeTest) {

	Engine &engine = *gEngine;

	Package *test = engine.package(Name(L"test.bs"));
	Scope tScope(capture(test));

	CHECK(test->find(Name(L"bar")));
	CHECK(test->find(Name(L"Foo")));
	CHECK(test->find(Name(L"Foo.a")));
	CHECK(test->find(Name(L"Foo.foo")));

	CHECK_RUNS(runFn(engine, L"test.bs.voidFn"));

	CHECK_EQ(runFn(engine, L"test.bs.bar"), 3);
	CHECK_EQ(runFn(engine, L"test.bs.ifTest", 1), 3);
	CHECK_EQ(runFn(engine, L"test.bs.ifTest", 2), 4);
	CHECK_EQ(runFn(engine, L"test.bs.ifTest", 3), 5);
	CHECK_EQ(runFn(engine, L"test.bs.ifTest2", 3), 4);
	CHECK_EQ(runFn(engine, L"test.bs.ifTest2", 0), -1);

	CHECK_EQ(runFn(engine, L"test.bs.assign", 1), 2);
	CHECK_EQ(runFn(engine, L"test.bs.while", 10), 1024);
	CHECK_EQ(runFn(engine, L"test.bs.for", 10), 1024);

	CHECK_EQ(runFn(engine, L"test.bs.createFoo"), 3);

	CHECK_ERROR(runFn(engine, L"test.bs.assignError"));

	CHECK_EQ(runFn(engine, L"test.bs.testBase"), 10);
	CHECK_EQ(runFn(engine, L"test.bs.testDerived"), 20);

	CHECK_EQ(runFn(engine, L"test.bs.testCpp", 1), 10);
	CHECK_EQ(runFn(engine, L"test.bs.testCpp", 2), 20);

	Auto<Object> created = runObjFn(engine, L"test.bs.createCpp", 1);
	CHECK_EQ(created.expect<Dbg>(engine, L"dbg")->get(), 10);
	created = runObjFn(engine, L"test.bs.createCpp", 2);
	CHECK_EQ(created.expect<Dbg>(engine, L"dbg")->get(), 20);

} END_TEST
