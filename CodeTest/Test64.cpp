#include "stdafx.h"

// Test 64-bit code.
BEGIN_TEST(Test64Asm) {
	Arena arena;
	Listing l;

	Variable v = l.frame.createLongVar(l.frame.root());
	Variable v2 = l.frame.createLongVar(l.frame.root());

	l << prolog();
	l << mov(v, longConst(0x123456789A));
	l << mov(v2, longConst(0xA987654321));
	l << add(v, v2);
	l << mov(rax, v);

	l << epilog();
	l << ret(8);

	Binary b(arena, L"Test64Asm");
	b.set(l);
	CHECK_EQ(callFn(b.getData(), int64(0)), 0xBBBBBBBBBB);

} END_TEST


BEGIN_TEST(Test64Preserve) {
	Arena arena;
	Listing l;

	Variable v = l.frame.createLongVar(l.frame.root());
	Variable v2 = l.frame.createLongVar(l.frame.root());

	l << prolog();
	l << mov(v, longConst(0x123456789A));
	l << mov(v2, longConst(0xA987654321));
	l << mov(rax, v);
	l << add(v, v2);
	l << epilog();
	l << ret(8);

	Binary b(arena, L"Test64Preserve");
	b.set(l);
	CHECK_EQ(callFn(b.getData(), int64(0)), 0x123456789A);
} END_TEST

