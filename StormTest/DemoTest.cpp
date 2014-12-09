#include "stdafx.h"
#include "Test/Test.h"
#include "Storm/Function.h"
#include "Code/Function.h"

BEGIN_TESTX(DemoTest) {

	Path root = Path::executable() + Path(L"../root/");
	Engine engine(root);

	Package *test = engine.package(Name(L"test.bs"));

	CHECK(test->find(Name(L"bar")));
	CHECK(test->find(Name(L"Foo")));

	Function *fun = as<Function>(engine.scope()->find(Name(L"test.bs.demo"), vector<Value>()));
	void *fn = fun->pointer();
	if (!fn)
		break;

	Int r = code::FnCall().call<Int>(fn);
	PLN("Function test.bs.demo() returns: " << r);

} END_TEST
