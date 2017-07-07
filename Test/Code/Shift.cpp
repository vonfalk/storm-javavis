#include "stdafx.h"
#include "Code/Binary.h"
#include "Code/Listing.h"

using namespace code;

BEGIN_TEST(ShiftTest, Code) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);

	Listing *l = new (e) Listing();

	Var p1 = l->createIntParam();
	Var p2 = l->createIntParam();

	*l << prolog();

	*l << mov(eax, p2);
	*l << mov(ecx, p1);
	*l << shl(ecx, al);
	*l << mov(eax, ecx);

	*l << mov(ecx, eax);
	*l << shr(ecx, byteRel(p2, Offset()));
	*l << bor(eax, ecx);

	*l << shl(eax, byteConst(1));
	*l << shr(eax, byteConst(1));

	*l << shl(eax, byteConst(2));
	*l << shr(eax, byteConst(2));

	*l << epilog();
	*l << ret(ValType(Size::sInt, false));

	Binary *b = new (e) Binary(arena, l);
	typedef Int (*Fn)(Int, Int);
	Fn fn = (Fn)b->address();

	CHECK_EQ((*fn)(1, 16), 0x10001);
} END_TEST
