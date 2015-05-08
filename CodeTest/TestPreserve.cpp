#include "stdafx.h"

BEGIN_TEST(TestPreserve) {
	Arena arena;
	Listing l;

	l << prolog();
	l << mov(rax, wordConst(0x00));
	l << mov(rbx, wordConst(0x01));
	l << mov(rcx, wordConst(0x02));
	l << epilog();
	l << ret(Size::sInt);

	Binary b(arena, l);
	CHECK_EQ(callFn(b.address(), 0), 0x00);
} END_TEST

static void dummy(void *ptr) {}

BEGIN_TEST(TestPreserveEx) {
	Arena arena;

	Ref destroyPtr = arena.external(L"dummy", &dummy);

	Listing l;

	l.frame.createPtrVar(l.frame.root(), destroyPtr);

	l << prolog();
	l << mov(rax, wordConst(0x00));
	l << mov(rbx, wordConst(0x01));
	l << mov(rcx, wordConst(0x02));
	l << epilog();
	l << ret(Size::sInt);

	Binary b(arena, l);
	CHECK_EQ(callFn(b.address(), 0), 0x00);
} END_TEST
