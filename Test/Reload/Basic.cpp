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
	reloadFile(reload, S("member.bs"), contents);
	CHECK_EQ(runFn<Int>(S("tests.reload.testClass"), 1), 22);

} END_TEST

BEGIN_TEST_(ClassType, Reload) {
	Engine &e = gEngine();

	Package *reload = e.package(S("tests.reload"));

	const char contents[] =
		"class TestClass {"
		"  init(Int x) {"
		"    init { x = x; }"
		"  }"
		"  Int x;"
		"  void update() {"
		"    x += 2;"
		"  }"
		"  void toS(StrBuf to) {"
		"    to << \"B\" << x;"
		"  }"
		"}";

	Object *first = runFn<Object *>(S("tests.reload.createClass"), 10);
	CHECK_EQ(::toS(first), L"A11");
	PVAR(first);

	reloadFile(reload, S("class.bs"), contents);
	Object *second = runFn<Object *>(S("tests.reload.createClass"), 20);

	PVAR(first);
	PVAR(second);

	// Same GcType?
	CHECK_EQ(Gc::typeOf(first), Gc::typeOf(second));
	// Same vtable?
	CHECK_EQ(vtable::from(first), vtable::from(second));

	// Check behavior.
	CHECK_EQ(::toS(first), L"B11");
	CHECK_EQ(::toS(second), L"B22");


} END_TEST
