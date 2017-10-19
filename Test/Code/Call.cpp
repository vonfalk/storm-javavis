#include "stdafx.h"
#include "Code/Binary.h"
#include "Code/Listing.h"
#include "Code/X64/Asm.h"
#include "Compiler/Debug.h"

using namespace code;

// If this is static, it seems the compiler optimizes it away, which breaks stuff.
Int CODECALL callIntFn(Int v) {
	return v + 2;
}

BEGIN_TEST(CallPrimitive, Code) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);

	Ref intFn = arena->external(S("intFn"), address(&callIntFn));

	Listing *l = new (e) Listing();
	l->result = intDesc(e);
	*l << prolog();

	*l << fnParam(intDesc(e), intConst(100));
	*l << fnCall(intFn, false, intDesc(e), ecx);

	*l << fnRet(ecx);

	Binary *b = new (e) Binary(arena, l);
	typedef Int (*Fn)();
	Fn fn = (Fn)b->address();

	CHECK_EQ((*fn)(), 102);
} END_TEST

BEGIN_TEST(CallPrimitiveRef, Code) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);

	Ref intFn = arena->external(S("intFn"), address(&callIntFn));

	Listing *l = new (e) Listing();
	l->result = intDesc(e);
	Var v = l->createIntVar(l->root());
	*l << prolog();

	*l << mov(v, intConst(100));
	*l << lea(ptrA, v);
	*l << fnParamRef(intDesc(e), ptrA);
	*l << fnCall(intFn, false, intDesc(e), ecx);

	*l << fnRet(ecx);

	Binary *b = new (e) Binary(arena, l);
	typedef Int (*Fn)();
	Fn fn = (Fn)b->address();

	CHECK_EQ((*fn)(), 102);
} END_TEST

Int CODECALL callIntManyFn(Int a, Int b, Int c, Int d, Int e, Int f, Int g, Int h) {
	return a*10000000 + b*1000000 + c*100000 + d*10000 + e*1000 + f*100 + g*10 + h;
}

BEGIN_TEST(CallPrimitiveMany, Code) {
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
	*l << fnCall(intFn, false, intDesc(e), ecx);

	*l << fnRet(ecx);

	Binary *b = new (e) Binary(arena, l);
	typedef Int (*Fn)();
	Fn fn = (Fn)b->address();

	CHECK_EQ((*fn)(), 34567281);
} END_TEST

#ifdef X64

// Only usable on X86-64, as we rely on platform specific registers.
BEGIN_TEST(CallPrimitiveMany64, Code) {
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
	*l << fnCall(intFn, false, intDesc(e), ecx);

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

BEGIN_TEST(CallSmallInt, Code) {
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
	*l << fnCall(intFn, false, intDesc(e), eax);

	*l << fnRet(eax);

	Binary *b = new (e) Binary(arena, l);
	typedef Int (*Fn)();
	Fn fn = (Fn)b->address();

	CHECK_EQ((*fn)(), 50);
} END_TEST

BEGIN_TEST(CallSmallIntRef, Code) {
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

	*l << lea(ptrA, v);
	*l << fnParamRef(desc, ptrA);
	*l << fnCall(intFn, false, intDesc(e), eax);

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

BEGIN_TEST(CallMixedInt, Code) {
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
	*l << fnCall(intFn, false, intDesc(e), eax);

	*l << fnRet(eax);

	Binary *bin = new (e) Binary(arena, l);
	typedef Int (*Fn)();
	Fn fn = (Fn)bin->address();

	CHECK_EQ((*fn)(), 30);
} END_TEST

BEGIN_TEST(CallMixedIntRef, Code) {
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

	*l << lea(ptrA, a);
	*l << lea(ptrB, b);

	*l << fnParamRef(large, ptrA);
	*l << fnParamRef(small, ptrB);
	*l << fnCall(intFn, false, intDesc(e), eax);

	*l << fnRet(eax);

	Binary *bin = new (e) Binary(arena, l);
	typedef Int (*Fn)();
	Fn fn = (Fn)bin->address();

	CHECK_EQ((*fn)(), 30);
} END_TEST

struct MixedParam {
	size_t a;
	Float b;
	Float c;
};

SimpleDesc *mixedDesc(Engine &e) {
	SimpleDesc *desc = new (e) SimpleDesc(Size::sPtr + Size::sFloat * 2, 3);
	desc->at(0) = Primitive(primitive::integer, Size::sPtr, Offset());
	desc->at(1) = Primitive(primitive::real, Size::sFloat, Offset::sPtr);
	desc->at(2) = Primitive(primitive::real, Size::sFloat, Offset::sPtr + Offset::sFloat);
	return desc;
}

Float CODECALL callMixed(MixedParam a) {
	return Float(a.a) - a.b / a.c;
}

BEGIN_TEST(CallMixed, Code) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);

	Ref mixedFn = arena->external(S("mixedFn"), address(&callMixed));
	SimpleDesc *mixed = mixedDesc(e);

	Listing *l = new (e) Listing();
	l->result = floatDesc(e);
	Var a = l->createVar(l->root(), mixed->size());

	*l << prolog();

	*l << mov(ptrRel(a, Offset()), ptrConst(100));
	*l << mov(floatRel(a, Offset::sPtr), floatConst(40.0f));
	*l << mov(floatRel(a, Offset::sPtr + Offset::sFloat), floatConst(10.0f));

	*l << fnParam(mixed, a);
	*l << fnCall(mixedFn, false, floatDesc(e), eax);

	*l << fnRet(eax);

	Binary *bin = new (e) Binary(arena, l);
	typedef Float (*Fn)();
	Fn fn = (Fn)bin->address();

	CHECK_EQ((*fn)(), 96.0f);
} END_TEST

BEGIN_TEST(CallMixedRef, Code) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);

	Ref mixedFn = arena->external(S("mixedFn"), address(&callMixed));
	SimpleDesc *mixed = mixedDesc(e);

	Listing *l = new (e) Listing();
	l->result = floatDesc(e);
	Var a = l->createVar(l->root(), mixed->size());

	*l << prolog();

	*l << mov(ptrRel(a, Offset()), ptrConst(100));
	*l << mov(floatRel(a, Offset::sPtr), floatConst(40.0f));
	*l << mov(floatRel(a, Offset::sPtr + Offset::sFloat), floatConst(10.0f));

	*l << lea(ptrA, a);
	*l << fnParamRef(mixed, ptrA);
	*l << fnCall(mixedFn, false, floatDesc(e), eax);

	*l << fnRet(eax);

	Binary *bin = new (e) Binary(arena, l);
	typedef Float (*Fn)();
	Fn fn = (Fn)bin->address();

	CHECK_EQ((*fn)(), 96.0f);
} END_TEST

Int CODECALL callComplex(storm::debug::DbgVal dbg, Int v) {
	return dbg.v + v;
}

BEGIN_TEST(CallComplex, Code) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);
	Type *dbgVal = storm::debug::DbgVal::stormType(e);

	Ref toCall = arena->external(S("callComplex"), address(&callComplex));
	ComplexDesc *desc = new (e) ComplexDesc(dbgVal->size(), dbgVal->copyCtor()->ref(), dbgVal->destructor()->ref());

	Listing *l = new (e) Listing();
	l->result = intDesc(e);
	Var a = l->createVar(l->root(), desc);

	*l << prolog();

	*l << lea(ptrA, a);
	*l << fnParam(ptrDesc(e), ptrA);
	*l << fnCall(dbgVal->defaultCtor()->ref(), false);

	*l << mov(ecx, intConst(8));

	*l << fnParam(desc, a);
	*l << fnParam(intDesc(e), ecx);
	*l << fnCall(toCall, false, intDesc(e), eax);

	*l << fnRet(eax);

	Binary *b = new (e) Binary(arena, l);
	typedef Int (*Fn)();
	Fn fn = (Fn)b->address();

	storm::debug::DbgVal::clear();
	CHECK_EQ((*fn)(), 18);
	CHECK(storm::debug::DbgVal::clear());
} END_TEST

BEGIN_TEST(CallRefComplex, Code) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);
	Type *dbgVal = storm::debug::DbgVal::stormType(e);

	Ref toCall = arena->external(S("callComplex"), address(&callComplex));
	ComplexDesc *desc = new (e) ComplexDesc(dbgVal->size(), dbgVal->copyCtor()->ref(), dbgVal->destructor()->ref());

	Listing *l = new (e) Listing();
	l->result = intDesc(e);
	Var a = l->createVar(l->root(), desc);

	*l << prolog();

	*l << lea(ptrA, a);
	*l << fnParam(ptrDesc(e), ptrA);
	*l << fnCall(dbgVal->defaultCtor()->ref(), false);

	*l << mov(ecx, intConst(8));
	*l << lea(ptrA, a);
	*l << fnParamRef(desc, ptrA);
	*l << fnParam(intDesc(e), ecx);
	*l << fnCall(toCall, false, intDesc(e), eax);

	*l << fnRet(eax);

	Binary *b = new (e) Binary(arena, l);
	typedef Int (*Fn)();
	Fn fn = (Fn)b->address();

	storm::debug::DbgVal::clear();
	CHECK_EQ((*fn)(), 18);
	CHECK(storm::debug::DbgVal::clear());
} END_TEST
