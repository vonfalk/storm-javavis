#include "stdafx.h"
#include "Code/Binary.h"
#include "Code/Listing.h"

using namespace code;

// Returning boolean values may be tricky in some cases, as they only
// occupy the least significant byte of a machine word. On some architectures
// the higher bits needs to be cleared before return.
BEGIN_TEST(BoolTest, Code) {
	Engine &e = gEngine();

	Arena *arena = code::arena(e);
	Listing *l = new (e) Listing();
	Variable p = l->createIntParam();

	*l << prolog();

	// Make sure we clobber the high parts of the register.
	*l << mov(eax, p);
	*l << cmp(eax, eax);
	*l << setCond(al, ifEqual);

	*l << epilog();
	*l << ret(ValType(Size::sByte, false));

	Binary *b = new (e) Binary(arena, l);
	typedef bool (*Fn)(Int);
	Fn fn = (Fn)b->address();

	CHECK((*fn)(0));
	CHECK((*fn)(1));
	CHECK_EQ((*fn)(-1), (*fn)(1));

} END_TEST
