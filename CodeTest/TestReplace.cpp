#include "stdafx.h"
#include "Test/Test.h"

static cpuInt myFn() {
	return 10;
}

static cpuInt paramFn(cpuInt a, cpuInt b) {
	return a + b;
}

BEGIN_TEST(TestReplace) {
	Arena arena;

	Ref fun = arena.external(L"myFn", &myFn);
	Ref paramFun = arena.external(L"paramFn", &paramFn);

	Listing l2;
	l2 << call(fun, Size::sPtr);
	l2 << ret(Size::sPtr);

	Binary *middleBlob = new Binary(arena, l2);
	RefSource middle(arena, L"middle");
	middle.set(middleBlob);

	Listing l;
	Label lbl = l.label();

	l << jmp(lbl);
	l << call(fun, Size::sPtr);
	l << lbl;
	l << call(Ref(middle), Size::sPtr);
	l << ret(Size::sPtr);

	Binary output(arena, l);

	typedef cpuInt (*Fn)();
	Fn p = (Fn)output.address();
	CHECK_EQ((*p)(), 10);

	l2 = Listing();
	l2 << fnParam(natConst(128));
	l2 << fnParam(natConst(20));
	l2 << fnCall(paramFun, Size::sPtr);
	l2 << ret(Size::sPtr);
	Binary *middle2 = new Binary(arena, l2);
	middle.set(middle2);

	p = (Fn)output.address();
	CHECK_EQ((*p)(), 148);


} END_TEST
