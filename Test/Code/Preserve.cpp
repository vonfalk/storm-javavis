#include "stdafx.h"
#include "Code/Binary.h"
#include "Code/Listing.h"

using namespace code;

BEGIN_TEST(PreserveTest, Code) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);

	Listing *l = new (e) Listing();

	*l << prolog();

	*l << mov(rax, wordConst(0x00));
	*l << mov(rbx, wordConst(0x01));
	*l << mov(rcx, wordConst(0x02));

	l->result = intDesc(e);
	*l << fnRet(eax);

	Binary *b = new (e) Binary(arena, l);
	CHECK_EQ(callFn(b->address(), 0), 0x00);
} END_TEST

BEGIN_TEST(Preserve64, Code) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);

	Listing *l = new (e) Listing();
	Var v = l->createLongVar(l->root());
	Var w = l->createLongVar(l->root());

	*l << prolog();

	*l << mov(v, longConst(0x123456789A));
	*l << mov(w, longConst(0xA987654321));
	*l << mov(rax, v);
	*l << add(v, w);

	l->result = longDesc(e);
	*l << fnRet(rax);

	Binary *b = new (e) Binary(arena, l);
	CHECK_EQ(callFn(b->address(), int64(0)), 0x123456789A);

} END_TEST
