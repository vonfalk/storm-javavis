#include "stdafx.h"
#include "Code/Binary.h"
#include "Code/Listing.h"

using namespace code;

static Int copied = 0;

void CODECALL callCopyInt(Int *dest, Int *src) {
	*dest = *src;
	copied = *src;
}

// If this is static, it seems the compiler optimizes it away, which breaks stuff.
Int CODECALL callIntFn(Int v) {
	return v + 2;
}

BEGIN_TEST(CallCopyTest, Code) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);

	Ref copy = arena->external(L"copyInt", &callCopyInt);
	Ref intFn = arena->external(L"intFn", &callIntFn);

	Listing *l = new (e) Listing();
	Variable v = l->createIntVar(l->root());

	*l << prolog();

	*l << mov(v, intConst(20));
	*l << fnParam(v, copy);
	*l << fnCall(intFn, valInt());

	*l << epilog();
	*l << ret(valInt());

	Binary *b = new (e) Binary(arena, l);
	typedef Int (*Fn)();
	Fn fn = (Fn)b->address();

	CHECK_EQ((*fn)(), 22);
	CHECK_EQ(copied, 20);

} END_TEST


BEGIN_TEST(CallRefTest, Code) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);

	Ref copy = arena->external(L"copyInt", &callCopyInt);
	Ref intFn = arena->external(L"intFn", &callIntFn);

	Listing *l = new (e) Listing();
	Variable v = l->createIntVar(l->root());

	*l << prolog();

	*l << mov(v, intConst(20));
	*l << lea(ptrA, v);
	*l << fnParamRef(ptrA, Size::sInt, copy);
	*l << fnCall(intFn, valInt());

	*l << epilog();
	*l << ret(valInt());

	Binary *b = new (e) Binary(arena, l);
	typedef Int (*Fn)();
	Fn fn = (Fn)b->address();

	CHECK_EQ((*fn)(), 22);
	CHECK_EQ(copied, 20);

} END_TEST

BEGIN_TEST(CallRefPlainTest, Code) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);

	Ref intFn = arena->external(L"intFn", &callIntFn);

	Listing *l = new (e) Listing();
	Variable v = l->createIntVar(l->root());

	*l << prolog();

	*l << mov(v, intConst(20));
	*l << lea(ptrA, v);
	*l << fnParamRef(ptrA, Size::sInt);
	*l << fnCall(intFn, valInt());

	*l << epilog();
	*l << ret(valInt());

	Binary *b = new (e) Binary(arena, l);
	typedef Int (*Fn)();
	Fn fn = (Fn)b->address();

	CHECK_EQ((*fn)(), 22);
	CHECK_EQ(copied, 20);

} END_TEST
