#include "stdafx.h"
#include "Test/Test.h"

BEGIN_TEST(TestMul) {
	Arena arena;

	Listing l;
	Variable p1 = l.frame.createIntParam();
	Variable p2 = l.frame.createIntParam();

	l << prolog();
	l << mul(p1, p2);
	l << mov(eax, p1);
	l << epilog();
	l << ret(retVal(Size::sInt, false));

	Binary b(arena, l);
	typedef cpuInt (*F)(cpuInt, cpuInt);
	F f = (F)b.address();

	CHECK_EQ((*f)(2, 2), 4);
	CHECK_EQ((*f)(-2, -2), 4);
	CHECK_EQ((*f)(-2, 2), -4);

} END_TEST
