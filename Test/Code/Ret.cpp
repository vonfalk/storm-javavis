#include "stdafx.h"
#include "Code/Binary.h"
#include "Code/Listing.h"

using namespace code;

BEGIN_TEST_(SimpleRet, Code) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);

	Listing *l = new (e) Listing();
	*l << prolog();

	*l << mov(ecx, intConst(100));

	*l << fnRet(intDesc(e), ecx);

	Binary *b = new (e) Binary(arena, l);
	typedef Int (*Fn)();
	Fn fn = (Fn)b->address();

	CHECK_EQ((*fn)(), 100);
} END_TEST

BEGIN_TEST_(SimpleRetRef, Code) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);

	Listing *l = new (e) Listing();
	Var t = l->createIntVar(l->root());

	*l << prolog();

	*l << mov(t, intConst(100));
	*l << lea(ptrA, t);

	*l << fnRetRef(intDesc(e), ptrA);

	Binary *b = new (e) Binary(arena, l);
	typedef Int (*Fn)();
	Fn fn = (Fn)b->address();

	CHECK_EQ((*fn)(), 100);
} END_TEST
