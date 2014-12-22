#include "stdafx.h"
#include "Test/Test.h"
#include "Storm/Function.h"
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

BEGIN_TEST(CodeTest) {

	Path root = Path::executable() + Path(L"../root/");
	Engine engine(root);

	Package *test = engine.package(Name(L"test.bs"));

	CHECK(test->find(Name(L"bar")));
	CHECK(test->find(Name(L"Foo")));

	CHECK_EQ(runFn(engine, L"test.bs.bar"), 3);
	CHECK_EQ(runFn(engine, L"test.bs.ifTest", 1), 3);
	CHECK_EQ(runFn(engine, L"test.bs.ifTest", 2), 4);
	CHECK_EQ(runFn(engine, L"test.bs.ifTest", 3), 5);
	CHECK_EQ(runFn(engine, L"test.bs.ifTest2", 3), 4);
	CHECK_EQ(runFn(engine, L"test.bs.ifTest2", 0), -1);

	CHECK_EQ(runFn(engine, L"test.bs.assign", 1), 2);
	CHECK_EQ(runFn(engine, L"test.bs.while", 10), 1024);

} END_TEST
