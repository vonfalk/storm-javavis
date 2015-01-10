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
	Scope tScope(capture(test));

	CHECK(test->find(Name(L"bar")));
	CHECK(test->find(Name(L"Foo")));
	CHECK(test->find(Name(L"Foo.a")));
	CHECK(test->find(Name(L"Foo.foo")));

	// Try to create an object.
	Function *fooCtor = as<Function>(tScope.find(Name(L"Foo") + Name(Type::CTOR), vector<Value>(1, Value())));
	CHECK(fooCtor);
	Auto<Object> o = create<Object>(fooCtor, code::FnCall());
	PLN(o);
	CHECK(o.borrow());

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

	CHECK_ERROR(runFn(engine, L"test.bs.assignError"));

} END_TEST
