#include "stdafx.h"
#include "Test/Lib/Test.h"

// Returning boolean values may be tricky in some cases, as they only
// occupy the least significant byte of a machine word. On some architectures
// the higher bits needs to be cleared before return.

BEGIN_TEST(TestBool) {
	Arena arena;

	Listing l;
	Variable p = l.frame.createIntParam();

	l << prolog();

	// Make sure we make the register 'dirty'.
	l << mov(eax, p);
	l << cmp(eax, eax);
	l << setCond(al, ifEqual);

	l << epilog();
	l << ret(retVal(Size::sByte, false));

	Binary b(arena, l);
	typedef bool (*Fn)(cpuInt);
	Fn fn = (Fn)b.address();

	CHECK((*fn)(0));
	CHECK((*fn)(-1));

	CHECK_EQ((*fn)(-1), (*fn)(1));

} END_TEST