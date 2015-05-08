#include "stdafx.h"
#include "Test/Test.h"

BEGIN_TEST(StoreRef) {
	Arena arena;

	RefSource src(arena, L"dummy");
	src.setPtr((void *)0xDEADBEEF);

	Listing l;

	l << prolog();
	// Note: lea of a reference has a special meaning. It loads the
	// index of the reference instead of the reference itself.
	l << lea(ptrA, src);
	l << epilog();
	l << ret(Size::sPtr);

	Binary b(arena, l);
	typedef void *(* Fn)();
	Fn fn = (Fn)b.address();
	void *r = (*fn)();

	Ref reclaimed = Ref::fromLea(arena, r);
	CHECK_EQ(reclaimed.address(), (void *)0xDEADBEEF);

} END_TEST
