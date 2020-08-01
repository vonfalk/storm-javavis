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
	// PVAR(first);

	reloadFile(reload, S("class.bs"), contents);
	Object *second = runFn<Object *>(S("tests.reload.createClass"), 20);

	// PVAR(first);
	// PVAR(second);

	// Same GcType?
	CHECK_EQ(Gc::typeOf(first), Gc::typeOf(second));
	// Same vtable?
	CHECK_EQ(vtable::from(first), vtable::from(second));

	// Check behavior.
	CHECK_EQ(::toS(first), L"B11");
	CHECK_EQ(::toS(second), L"B22");


} END_TEST

BEGIN_TEST_(ClassInherit, Reload) {
	Engine &e = gEngine();

	Package *reload = e.package(S("tests.reload"));

	const char contents[] =
		"class Base {"
		"  Int x;"
		"  void toS(StrBuf to) {"
		"    to << \"B'\" << x;"
		"  }"
		"}"
		"class Derived1 extends Base {"
		"  void toS(StrBuf to) {"
		"    super:toS(to);"
		"    to << \"D'\" << x;"
		"  }"
		"}";

	Object *first1 = runFn<Object *>(S("tests.reload.inhBase"), 1);
	Object *first2 = runFn<Object *>(S("tests.reload.inhDerived1"), 2);
	Object *first3 = runFn<Object *>(S("tests.reload.inhDerived2"), 3);

	CHECK_EQ(::toS(first1), L"B1");
	CHECK_EQ(::toS(first2), L"B2D2");
	CHECK_EQ(::toS(first3), L"B3E3");

	// Make sure these functions are compiled before reloading.
	CHECK_EQ(runFn<Bool>(S("tests.reload.isBase"), first1), true);
	CHECK_EQ(runFn<Bool>(S("tests.reload.isDerived1"), first1), false);
	CHECK_EQ(runFn<Bool>(S("tests.reload.isDerived2"), first1), false);

	reloadFile(reload, S("inheritance.bs"), contents);

	CHECK_EQ(::toS(first1), L"B'1");
	CHECK_EQ(::toS(first2), L"B'2D'2");
	CHECK_EQ(::toS(first3), L"B'3E3");

	{
		CHECK_EQ(runFn<Bool>(S("tests.reload.isBase"), first1), true);
		CHECK_EQ(runFn<Bool>(S("tests.reload.isDerived1"), first1), false);
		CHECK_EQ(runFn<Bool>(S("tests.reload.isDerived2"), first1), false);

		CHECK_EQ(runFn<Bool>(S("tests.reload.isBase"), first2), true);
		CHECK_EQ(runFn<Bool>(S("tests.reload.isDerived1"), first2), true);
		CHECK_EQ(runFn<Bool>(S("tests.reload.isDerived2"), first2), false);

		CHECK_EQ(runFn<Bool>(S("tests.reload.isBase"), first3), true);
		CHECK_EQ(runFn<Bool>(S("tests.reload.isDerived1"), first3), false);
		CHECK_EQ(runFn<Bool>(S("tests.reload.isDerived2"), first3), true);
	}

	Object *second1 = runFn<Object *>(S("tests.reload.inhBase"), 4);
	Object *second2 = runFn<Object *>(S("tests.reload.inhDerived1"), 5);
	Object *second3 = runFn<Object *>(S("tests.reload.inhDerived2"), 6);

	CHECK_EQ(::toS(second1), L"B'4");
	CHECK_EQ(::toS(second2), L"B'5D'5");
	CHECK_EQ(::toS(second3), L"B'6E6");

	{
		CHECK_EQ(runFn<Bool>(S("tests.reload.isBase"), second1), true);
		CHECK_EQ(runFn<Bool>(S("tests.reload.isDerived1"), second1), false);
		CHECK_EQ(runFn<Bool>(S("tests.reload.isDerived2"), second1), false);

		CHECK_EQ(runFn<Bool>(S("tests.reload.isBase"), second2), true);
		CHECK_EQ(runFn<Bool>(S("tests.reload.isDerived1"), second2), true);
		CHECK_EQ(runFn<Bool>(S("tests.reload.isDerived2"), second2), false);

		CHECK_EQ(runFn<Bool>(S("tests.reload.isBase"), second3), true);
		CHECK_EQ(runFn<Bool>(S("tests.reload.isDerived1"), second3), false);
		CHECK_EQ(runFn<Bool>(S("tests.reload.isDerived2"), second3), true);
	}

	CHECK_EQ(Gc::typeOf(first1), Gc::typeOf(second1));
	CHECK_EQ(Gc::typeOf(first2), Gc::typeOf(second2));
	CHECK_EQ(Gc::typeOf(first3), Gc::typeOf(second3));
	CHECK_EQ(vtable::from(first1), vtable::from(second1));
	CHECK_EQ(vtable::from(first2), vtable::from(second2));
	CHECK_EQ(vtable::from(first3), vtable::from(second3));


} END_TEST
