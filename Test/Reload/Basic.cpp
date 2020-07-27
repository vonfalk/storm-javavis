#include "stdafx.h"
#include "Util.h"

BEGIN_TEST_(Basic, Reload) {
	Engine &e = gEngine();

	Package *reload = e.package(S("tests.reload"));

	CHECK_EQ(runFn<Int>(S("tests.reload.testRoot"), 1), 2);
	reloadFile(reload, S("fn.bs"), S("Int testA(Int x) { x + 2; }"));
	CHECK_EQ(runFn<Int>(S("tests.reload.testRoot"), 1), 3);

} END_TEST

BEGIN_TEST_(MemberFn, Reload) {
	Engine &e = gEngine();

	Package *reload = e.package(S("tests.reload"));

	const char contents[] =
		"class Replaced {"
		"  Int x;"
		"  init(Int x) {"
		"    init { x = x*2; }"
		"  }"
		"  Int compute() {"
		"    x * 11;"
		"  }"
		"}";

	CHECK_EQ(runFn<Int>(S("tests.reload.testClass"), 1), 10);
	reloadFile(reload, S("class.bs"), contents);
	CHECK_EQ(runFn<Int>(S("tests.reload.testClass"), 1), 22);

} END_TEST
