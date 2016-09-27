#include "stdafx.h"
#include "Test/Lib/Test.h"

BEGIN_TEST(TestParamMove) {
	Arena arena;

	Listing l;
	Variable p1 = l.frame.createIntParam();
	Variable p2 = l.frame.createIntParam();

	l.frame.moveParam(p2, 0);

	l << prolog();
	l << mov(eax, p2);
	l << epilog();
	l << ret(retVal(Size::sInt, false));

	Binary b(arena, l);
	typedef Int (*F)(Int, Int);
	F f = (F)b.address();

	CHECK_EQ((*f)(1, 2), 1);

} END_TEST
