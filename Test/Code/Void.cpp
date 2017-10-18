#include "stdafx.h"
#include "Code/Binary.h"
#include "Code/Listing.h"

using namespace code;

static int tmpVar = 0;

static void CODECALL voidFn(int c) {
	tmpVar = c;
}

BEGIN_TEST(VoidTest, Code) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);
	Ref r = arena->external(S("voidFn"), address(&voidFn));

	Listing *l = new (e) Listing();

	*l << prolog();

	*l << fnParam(intDesc(e), intConst(3));
	*l << fnCall(r, false);

	*l << fnRet();

	Binary *b = new (e) Binary(arena, l);
	typedef void (*Fn)();
	Fn fn = (Fn)b->address();

	tmpVar = 0;
	(*fn)();
	CHECK_EQ(tmpVar, 3);

} END_TEST
