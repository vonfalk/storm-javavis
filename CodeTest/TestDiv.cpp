#include "stdafx.h"
#include "Test/Test.h"

BEGIN_TEST(TestIDiv) {
	Arena arena;

	Listing l;
	Variable p1 = l.frame.createIntParam();
	Variable p2 = l.frame.createIntParam();

	l << prolog();
	l << idiv(p1, p2);
	l << mov(eax, p1);
	l << epilog();
	l << ret(Size::sInt);

	Binary b(arena, l);
	typedef cpuInt (*F)(cpuInt, cpuInt);
	F f = (F)b.address();

	CHECK_EQ((*f)(4, 2), 2);
	CHECK_EQ((*f)(-8, 2), -4);
	CHECK_EQ((*f)(-8, -2), 4);

} END_TEST

BEGIN_TEST(TestUDiv) {
	Arena arena;

	Listing l;
	Variable p1 = l.frame.createIntParam();
	Variable p2 = l.frame.createIntParam();

	l << prolog();
	l << udiv(p1, p2);
	l << mov(eax, p1);
	l << epilog();
	l << ret(Size::sInt);

	Binary b(arena, l);
	typedef cpuInt (*F)(cpuInt, cpuInt);
	F f = (F)b.address();

	CHECK_EQ((*f)(4, 2), 2);
	CHECK_EQ((*f)(0x80000000, 2), 0x40000000);

} END_TEST

BEGIN_TEST(TestIMod) {
	Arena arena;

	Listing l;
	Variable p1 = l.frame.createIntParam();
	Variable p2 = l.frame.createIntParam();

	l << prolog();
	l << imod(p1, p2);
	l << mov(eax, p1);
	l << epilog();
	l << ret(Size::sInt);

	Binary b(arena, l);
	typedef cpuInt (*F)(cpuInt, cpuInt);
	F f = (F)b.address();

	CHECK_EQ((*f)(18, 10), 8);
	CHECK_EQ((*f)(-18, 10), -8);
	CHECK_EQ((*f)(-18, -10), -8);

} END_TEST

BEGIN_TEST(TestUMod) {
	Arena arena;

	Listing l;
	Variable p1 = l.frame.createIntParam();
	Variable p2 = l.frame.createIntParam();

	l << prolog();
	l << umod(p1, p2);
	l << mov(eax, p1);
	l << epilog();
	l << ret(Size::sInt);

	Binary b(arena, l);
	typedef cpuInt (*F)(cpuInt, cpuInt);
	F f = (F)b.address();

	CHECK_EQ((*f)(14, 10), 4);
	CHECK_EQ((*f)(0x80000000, 3), 2);

} END_TEST
