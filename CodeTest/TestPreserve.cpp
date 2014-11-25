#include "stdafx.h"

BEGIN_TEST(TestPreserve) {
	Arena arena;
	Listing l;

	l << prolog();
	l << mov(eax, natConst(0x00));
	l << mov(ebx, natConst(0x01));
	l << mov(ecx, natConst(0x02));
	l << epilog();
	l << ret(4);

	Binary b(arena, L"TestPreserve", l);
	CHECK_EQ(callFn(b.getData(), 0), 0x00);
} END_TEST
