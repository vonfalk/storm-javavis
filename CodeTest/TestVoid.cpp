#include "stdafx.h"
#include "Test/Test.h"

static int tmpVar = 0;

void voidFn(int c) {
	tmpVar = c;
}

BEGIN_TEST(TestVoid) {
	Arena arena;

	Ref r = arena.external(L"VoidFn", &voidFn);

	Listing l;
	l << prolog();
	l << fnParam(intConst(3));
	l << fnCall(r, Size());
	l << epilog();
	l << ret(Size());

	Binary b(arena, l);

	tmpVar = 0;
	call<void>(b.address(), false);
	CHECK_EQ(tmpVar, 3);

} END_TEST
