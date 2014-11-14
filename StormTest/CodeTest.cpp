#include "stdafx.h"
#include "Test/Test.h"

BEGIN_TEST(CodeTest) {

	Path root = Path::executable() + Path(L"../root/");
	Engine engine(root);

	Package *test = engine.package(Name(L"test.bs"));

	CHECK(test->find(Name(L"bar")));
	CHECK(test->find(Name(L"Foo")));

} END_TEST
