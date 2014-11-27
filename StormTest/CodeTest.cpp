#include "stdafx.h"
#include "Test/Test.h"
#include "Storm/Function.h"
#include "Code/Function.h"

BEGIN_TEST(CodeTest) {

	Path root = Path::executable() + Path(L"../root/");
	Engine engine(root);

	Package *test = engine.package(Name(L"test.bs"));

	CHECK(test->find(Name(L"bar")));
	CHECK(test->find(Name(L"Foo")));

	Function *fun = as<Function>(engine.scope()->find(Name(L"test.bs.bar"), vector<Value>()));
	CHECK(fun);
	void *fn = fun->pointer();
	CHECK(fn);
	if (fn) {
		CHECK_EQ(code::FnCall().call<Int>(fn), 3);
	}

} END_TEST
