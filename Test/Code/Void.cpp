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
	Ref r = arena->external(L"voidFn", &voidFn);

	Listing *l = new (e) Listing();

	*l << prolog();

	*l << fnParam(intConst(3));
	*l << fnCall(r, valVoid());

	*l << epilog();
	*l << ret(valVoid());

	Binary *b = new (e) Binary(arena, l);
	typedef Int (*Fn)();
	Fn fn = (Fn)b->address();

	tmpVar = 0;
	(*fn)();
	CHECK_EQ(tmpVar, 3);

} END_TEST
