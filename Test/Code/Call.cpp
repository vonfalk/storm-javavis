#include "stdafx.h"
#include "Code/Binary.h"
#include "Code/Listing.h"
#include "Code/X64/Asm.h"

using namespace code;

// If this is static, it seems the compiler optimizes it away, which breaks stuff.
Int CODECALL callIntFn(Int v) {
	return v + 2;
}

BEGIN_TEST_(CallPrimitive, Code) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);

	Ref intFn = arena->external(S("intFn"), address(&callIntFn));

	Listing *l = new (e) Listing();
	l->result = intDesc(e);
	*l << prolog();

	*l << fnParam(intDesc(e), intConst(100));
	*l << fnCall(intFn, intDesc(e), ecx);

	*l << fnRet(ecx);

	Binary *b = new (e) Binary(arena, l);
	typedef Int (*Fn)();
	Fn fn = (Fn)b->address();

	CHECK_EQ((*fn)(), 102);
} END_TEST

Int CODECALL callIntManyFn(Int a, Int b, Int c, Int d, Int e, Int f, Int g, Int h) {
	return a*10000000 + b*1000000 + c*100000 + d*10000 + e*1000 + f*100 + g*10 + h;
}

BEGIN_TEST_(CallPrimitiveMany, Code) {
	// Attempts to pass enough parameters to put at least one on the stack. Put some parameters in
	// register so that the backend has to reorder the register assignment to some degree.
	Engine &e = gEngine();
	Arena *arena = code::arena(e);

	Ref intFn = arena->external(S("intFn"), address(&callIntManyFn));

	Listing *l = new (e) Listing();
	l->result = intDesc(e);
	*l << prolog();

	*l << mov(eax, intConst(1));
	*l << mov(ecx, intConst(2));
	*l << fnParam(intDesc(e), intConst(3));
	*l << fnParam(intDesc(e), intConst(4));
	*l << fnParam(intDesc(e), intConst(5));
	*l << fnParam(intDesc(e), intConst(6));
	*l << fnParam(intDesc(e), intConst(7));
	*l << fnParam(intDesc(e), ecx);
	*l << fnParam(intDesc(e), intConst(8));
	*l << fnParam(intDesc(e), eax);
	*l << fnCall(intFn, intDesc(e), ecx);

	*l << fnRet(ecx);

	Binary *b = new (e) Binary(arena, l);
	typedef Int (*Fn)();
	Fn fn = (Fn)b->address();

	CHECK_EQ((*fn)(), 34567281);
} END_TEST

#ifdef X64

// Only usable on X86-64, as we rely on platform specific registers.
BEGIN_TEST_(CallPrimitiveMany64, Code) {
	// Call a function with parameters forming a cycle that the backend needs to break in order to
	// properly assign the registers.
	Engine &e = gEngine();
	Arena *arena = code::arena(e);

	Ref intFn = arena->external(S("intFn"), address(&callIntManyFn));

	Listing *l = new (e) Listing();
	l->result = intDesc(e);
	*l << prolog();

	using namespace code::x64;
	*l << mov(edi, intConst(1));
	*l << mov(esi, intConst(2));
	*l << mov(edx, intConst(3));
	*l << mov(ecx, intConst(4));
	*l << mov(e8,  intConst(5));
	*l << mov(e9,  intConst(6));

	*l << fnParam(intDesc(e), esi);
	*l << fnParam(intDesc(e), edx);
	*l << fnParam(intDesc(e), ecx);
	*l << fnParam(intDesc(e), e8);
	*l << fnParam(intDesc(e), e9);
	*l << fnParam(intDesc(e), edi);
	*l << fnParam(intDesc(e), intConst(0));
	*l << fnParam(intDesc(e), intConst(0));
	*l << fnCall(intFn, intDesc(e), ecx);

	*l << fnRet(ecx);

	Binary *b = new (e) Binary(arena, l);
	typedef Int (*Fn)();
	Fn fn = (Fn)b->address();

	CHECK_EQ((*fn)(), 23456100);
} END_TEST

#endif

struct SmallIntParam {
	size_t a;
	size_t b;
};

SimpleDesc *smallIntDesc(Engine &e) {
	SimpleDesc *desc = new (e) SimpleDesc(Size::sPtr * 2, 2);
	desc->at(0) = Primitive(primitive::integer, Size::sPtr, Offset());
	desc->at(1) = Primitive(primitive::integer, Size::sPtr, Offset::sPtr);
	return desc;
}

Int CODECALL callSmallInt(SmallIntParam p) {
	return Int(p.a + p.b);
}

BEGIN_TEST_(CallSmallInt, Code) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);

	Ref intFn = arena->external(S("intFn"), address(&callSmallInt));
	SimpleDesc *desc = smallIntDesc(e);

	Listing *l = new (e) Listing();
	l->result = intDesc(e);
	Var v = l->createVar(l->root(), desc->size());

	*l << prolog();

	*l << mov(ptrRel(v, Offset()), ptrConst(10));
	*l << mov(ptrRel(v, Offset::sPtr), ptrConst(40));

	*l << fnParam(desc, v);
	*l << fnCall(intFn, intDesc(e), eax);

	*l << fnRet(eax);

	Binary *b = new (e) Binary(arena, l);
	typedef Int (*Fn)();
	Fn fn = (Fn)b->address();

	CHECK_EQ((*fn)(), 50);
} END_TEST

struct LargeIntParam {
	size_t a;
	size_t b;
	size_t c;
};

SimpleDesc *largeIntDesc(Engine &e) {
	SimpleDesc *desc = new (e) SimpleDesc(Size::sPtr * 3, 3);
	desc->at(0) = Primitive(primitive::integer, Size::sPtr, Offset());
	desc->at(1) = Primitive(primitive::integer, Size::sPtr, Offset::sPtr);
	desc->at(2) = Primitive(primitive::integer, Size::sPtr, Offset::sPtr * 2);
	return desc;
}

Int CODECALL callMixedInt(LargeIntParam a, SmallIntParam b) {
	return Int(a.a + a.b - a.c + b.a - b.b);
}

BEGIN_TEST_(CallMixedInt, Code) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);

	Ref intFn = arena->external(S("intFn"), address(&callMixedInt));
	SimpleDesc *small = smallIntDesc(e);
	SimpleDesc *large = largeIntDesc(e);

	Listing *l = new (e) Listing();
	l->result = intDesc(e);
	Var a = l->createVar(l->root(), large->size());
	Var b = l->createVar(l->root(), small->size());

	*l << prolog();

	*l << mov(ptrRel(a, Offset()), ptrConst(1));
	*l << mov(ptrRel(a, Offset::sPtr), ptrConst(2));
	*l << mov(ptrRel(a, Offset::sPtr * 2), ptrConst(3));

	*l << mov(ptrRel(b, Offset()), ptrConst(40));
	*l << mov(ptrRel(b, Offset::sPtr), ptrConst(10));

	*l << fnParam(large, a);
	*l << fnParam(small, b);
	*l << fnCall(intFn, intDesc(e), eax);

	*l << fnRet(eax);

	Binary *bin = new (e) Binary(arena, l);
	typedef Int (*Fn)();
	Fn fn = (Fn)bin->address();

	CHECK_EQ((*fn)(), 30);
} END_TEST


static Int copied = 0;

void CODECALL callCopyInt(Int *dest, Int *src) {
	*dest = *src;
	copied = *src;
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
