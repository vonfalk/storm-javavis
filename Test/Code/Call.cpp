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

	Ref copy = arena->external(S("copyInt"), address(&callCopyInt));
	Ref intFn = arena->external(S("intFn"), address(&callIntFn));

	Listing *l = new (e) Listing();
	Var v = l->createIntVar(l->root());

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

	Ref copy = arena->external(S("copyInt"), address(&callCopyInt));
	Ref intFn = arena->external(S("intFn"), address(&callIntFn));

	Listing *l = new (e) Listing();
	Var v = l->createIntVar(l->root());

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

	Ref intFn = arena->external(S("intFn"), address(&callIntFn));

	Listing *l = new (e) Listing();
	Var v = l->createIntVar(l->root());

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

struct Large {
	size_t a, b;
};

void CODECALL largeCopy(Large *to, Large *src) {
	*to = *src;
}

size_t CODECALL largeFn(size_t a, Large b, size_t c) {
	return a + b.a + b.b + c;
}

BEGIN_TEST(CallRefMultiple, Code) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);

	Ref largeFn = arena->external(S("largeFn"), address(&::largeFn));
	Ref largeCopy = arena->external(S("largeCopy"), address(&::largeCopy));

	Listing *l = new (e) Listing();

	Var a = l->createVar(l->root(), Size::sPtr);
	Var b = l->createVar(l->root(), Size::sPtr*2);
	Var c = l->createVar(l->root(), Size::sPtr);

	Var pA = l->createVar(l->root(), Size::sPtr);
	Var pB = l->createVar(l->root(), Size::sPtr);
	Var pC = l->createVar(l->root(), Size::sPtr);


	*l << prolog();

	*l << mov(a, ptrConst(10));
	*l << mov(ptrRel(b, Offset()), ptrConst(1));
	*l << mov(ptrRel(b, Offset::sPtr), ptrConst(2));
	*l << mov(c, ptrConst(20));

	*l << lea(pA, a);
	*l << lea(pB, b);
	*l << lea(pC, c);

	*l << fnParamRef(pA, Size::sPtr);
	*l << fnParamRef(pB, Size::sPtr*2, largeCopy);
	*l << fnParamRef(pC, Size::sPtr);
	*l << fnCall(largeFn, valPtr());

	*l << epilog();
	*l << ret(valPtr());

	Binary *bin = new (e) Binary(arena, l);
	typedef size_t (*Fn)();
	Fn fn = (Fn)bin->address();

	CHECK_EQ((*fn)(), 33);
} END_TEST
