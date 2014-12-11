#include "stdafx.h"
#include "Test/Test.h"

BEGIN_TEST(TestShift) {
	Arena arena;
	Listing l;

	Variable p1 = l.frame.createIntParam();
	Variable p2 = l.frame.createIntParam();

	l << prolog();
	l << mov(eax, p2);
	l << mov(ecx, p1);
	l << shl(ecx, al);
	l << mov(eax, ecx);

	l << mov(ecx, eax);
	l << shr(ecx, byteRel(p2, 0));
	l << or(eax, ecx);

	l << shl(eax, byteConst(1));
	l << shr(eax, byteConst(1));

	l << shl(eax, byteConst(2));
	l << shr(eax, byteConst(2));

	l << epilog();
	l << ret(4);

	Binary b(arena, L"TestShift", l);
	typedef cpuInt (*F)(cpuInt, cpuInt);
	F f = (F)b.getData();

	CHECK_EQ((*f)(1, 16), 0x10001);

} END_TEST
