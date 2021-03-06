#include "stdafx.h"
#include "Code/Binary.h"
#include "Code/Listing.h"

using namespace code;

BEGIN_TEST(NotTest, Code) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);

	Listing *l = new (e) Listing();
	Var p = l->createIntParam();

	*l << prolog();

	*l << bnot(p);
	*l << mov(eax, p);

	l->result = intDesc(e);
	*l << fnRet(eax);

	Binary *b = new (e) Binary(arena, l);
	typedef Int (*Fn)(Int);
	Fn fn = (Fn)b->address();

	CHECK_EQ((*fn)(10), ~Int(10));

} END_TEST
