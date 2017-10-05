#include "stdafx.h"
#include "Code/Binary.h"
#include "Code/Listing.h"
#include "Compiler/Debug.h"

using namespace code;

BEGIN_TEST_(RetPrimitive, Code) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);

	Listing *l = new (e) Listing();
	l->result = intDesc(e);

	*l << prolog();

	*l << mov(ecx, intConst(100));

	*l << fnRet(ecx);

	Binary *b = new (e) Binary(arena, l);
	typedef Int (*Fn)();
	Fn fn = (Fn)b->address();

	CHECK_EQ((*fn)(), 100);
} END_TEST

BEGIN_TEST_(RetRefPrimitive, Code) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);

	Listing *l = new (e) Listing();
	l->result = intDesc(e);
	Var t = l->createIntVar(l->root());

	*l << prolog();

	*l << mov(t, intConst(100));
	*l << lea(ptrA, t);

	*l << fnRetRef(ptrA);

	Binary *b = new (e) Binary(arena, l);
	typedef Int (*Fn)();
	Fn fn = (Fn)b->address();

	CHECK_EQ((*fn)(), 100);
} END_TEST

BEGIN_TEST_(RetFloatPrimitive, Code) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);

	Listing *l = new (e) Listing();
	l->result = floatDesc(e);

	*l << prolog();

	*l << mov(ecx, floatConst(10.0f));

	*l << fnRet(ecx);

	Binary *b = new (e) Binary(arena, l);
	typedef Float (*Fn)();
	Fn fn = (Fn)b->address();

	CHECK_EQ((*fn)(), 10.0f);
} END_TEST

BEGIN_TEST_(RetComplex, Code) {
	using storm::debug::DbgVal;
	Engine &e = gEngine();
	Arena *arena = code::arena(e);
	Type *dbgVal = DbgVal::stormType(e);

	Listing *l = new (e) Listing();
	l->result = new (e) ComplexDesc(dbgVal->size(), dbgVal->copyCtor()->ref(), dbgVal->destructor()->ref());
	Var v = l->createVar(l->root(), dbgVal->size(), dbgVal->destructor()->ref(), freeDef | freePtr);

	*l << prolog();

	*l << lea(ptrA, v);
	*l << fnParam(ptrDesc(e), ptrA);
	*l << fnCall(dbgVal->defaultCtor()->ref());

	*l << fnRet(v);

	Binary *b = new (e) Binary(arena, l);
	typedef DbgVal (*Fn)();
	Fn fn = (Fn)b->address();

	DbgVal::clear();
	CHECK_EQ((*fn)().v, 10);
	CHECK(DbgVal::clear());
} END_TEST

BEGIN_TEST_(RetRefComplex, Code) {
	using storm::debug::DbgVal;
	Engine &e = gEngine();
	Arena *arena = code::arena(e);
	Type *dbgVal = DbgVal::stormType(e);

	Listing *l = new (e) Listing();
	l->result = new (e) ComplexDesc(dbgVal->size(), dbgVal->copyCtor()->ref(), dbgVal->destructor()->ref());
	Var v = l->createVar(l->root(), dbgVal->size(), dbgVal->destructor()->ref(), freeDef | freePtr);

	*l << prolog();

	*l << lea(ptrA, v);
	*l << fnParam(ptrDesc(e), ptrA);
	*l << fnCall(dbgVal->defaultCtor()->ref());

	*l << lea(ptrA, v);
	*l << fnRetRef(ptrA);

	Binary *b = new (e) Binary(arena, l);
	typedef DbgVal (*Fn)();
	Fn fn = (Fn)b->address();

	DbgVal::clear();
	CHECK_EQ((*fn)().v, 10);
	CHECK(DbgVal::clear());
} END_TEST

storm::debug::DbgVal STORM_FN createDbgVal() {
	return storm::debug::DbgVal(121);
}

BEGIN_TEST_(RetCallComplex, Code) {
	using storm::debug::DbgVal;
	Engine &e = gEngine();
	Arena *arena = code::arena(e);
	Type *dbgVal = DbgVal::stormType(e);
	TypeDesc *dbgDesc = new (e) ComplexDesc(dbgVal->size(), dbgVal->copyCtor()->ref(), dbgVal->destructor()->ref());
	Ref toCall = arena->external(S("create"), address(&createDbgVal));

	Listing *l = new (e) Listing();
	l->result = intDesc(e);
	Var v = l->createVar(l->root(), dbgVal->size(), dbgVal->destructor()->ref(), freeDef | freePtr);

	*l << prolog();

	*l << fnCall(toCall, dbgDesc, v);

	// This is abusin the interface a tiny bit...
	*l << mov(eax, intRel(v, Offset()));

	*l << fnRet(eax);

	Binary *b = new (e) Binary(arena, l);
	typedef Int (*Fn)();
	Fn fn = (Fn)b->address();

	DbgVal::clear();
	CHECK_EQ((*fn)(), 121);
	CHECK(DbgVal::clear());

} END_TEST

struct SimpleRet {
	size_t a;
	size_t b;
};

bool operator ==(const SimpleRet &a, const SimpleRet &b) {
	return a.a == b.a
		&& a.b == b.b;
}

bool operator !=(const SimpleRet &a, const SimpleRet &b) {
	return !(a == b);
}

wostream &operator <<(wostream &to, const SimpleRet &b) {
	return to << L"{ " << b.a << L", " << b.b << L" }";
}

SimpleDesc *retDesc(Engine &e) {
	SimpleDesc *desc = new (e) SimpleDesc(Size::sPtr * 2, 2);
	desc->at(0) = Primitive(primitive::integer, Size::sPtr, Offset());
	desc->at(1) = Primitive(primitive::integer, Size::sPtr, Offset::sPtr);
	return desc;
}

BEGIN_TEST_(RetSimple, Code) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);

	Listing *l = new (e) Listing();
	l->result = retDesc(e);
	Var v = l->createVar(l->root(), Size::sPtr * 2);

	*l << prolog();

	*l << lea(ptrA, v);
	*l << mov(ptrRel(ptrA, Offset()), ptrConst(10));
	*l << mov(ptrRel(ptrA, Offset::sPtr), ptrConst(20));

	*l << fnRet(v);

	Binary *b = new (e) Binary(arena, l);
	typedef SimpleRet (*Fn)();
	Fn fn = (Fn)b->address();

	SimpleRet ok = { 10, 20 };
	CHECK_EQ((*fn)(), ok);
} END_TEST

SimpleRet CODECALL createSimple() {
	SimpleRet a = { 100, 20 };
	return a;
}

BEGIN_TEST_(RetCallSimple, Code) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);
	Ref toCall = arena->external(S("create"), address(&createSimple));

	Listing *l = new (e) Listing();
	l->result = ptrDesc(e);
	Var v = l->createVar(l->root(), Size::sPtr * 2);

	*l << prolog();

	*l << fnCall(toCall, retDesc(e), v);
	*l << mov(ptrA, ptrRel(v, Offset()));
	*l << add(ptrA, ptrRel(v, Offset::sPtr));

	*l << fnRet(ptrA);

	Binary *b = new (e) Binary(arena, l);
	typedef size_t (*Fn)();
	Fn fn = (Fn)b->address();

	CHECK_EQ((*fn)(), 120);
} END_TEST

struct SimpleFloatRet {
	int a, b;
	float c;
};

bool operator ==(const SimpleFloatRet &a, const SimpleFloatRet &b) {
	return a.a == b.a
		&& a.b == b.b
		&& a.c == b.c;
}

bool operator !=(const SimpleFloatRet &a, const SimpleFloatRet &b) {
	return !(a == b);
}

wostream &operator <<(wostream &to, const SimpleFloatRet &b) {
	return to << L"{ " << b.a << L", " << b.b << L", " << b.c << L" }";
}

SimpleDesc *retFloatDesc(Engine &e) {
	SimpleDesc *desc = new (e) SimpleDesc(Size::sInt * 3, 3);
	desc->at(0) = Primitive(primitive::integer, Size::sInt, Offset());
	desc->at(1) = Primitive(primitive::integer, Size::sInt, Offset::sInt);
	desc->at(2) = Primitive(primitive::real, Size::sInt, Offset::sInt * 2);
	return desc;
}

BEGIN_TEST_(RetSimpleFloat, Code) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);

	Listing *l = new (e) Listing();
	l->result = retFloatDesc(e);
	Var v = l->createVar(l->root(), Size::sInt * 3);

	*l << prolog();

	*l << lea(ptrA, v);
	*l << mov(intRel(ptrA, Offset()), intConst(10));
	*l << mov(intRel(ptrA, Offset::sInt), intConst(20));
	*l << mov(intRel(ptrA, Offset::sInt * 2), floatConst(20.5f));

	*l << fnRet(v);

	Binary *b = new (e) Binary(arena, l);
	typedef SimpleFloatRet (*Fn)();
	Fn fn = (Fn)b->address();

	SimpleFloatRet ok = { 10, 20, 20.5f };
	CHECK_EQ((*fn)(), ok);
} END_TEST

SimpleFloatRet CODECALL createSimpleFloat() {
	SimpleFloatRet x = { 10, 20, 3.8f };
	return x;
}

BEGIN_TEST_(RetCallSimpleFloat, Code) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);
	Ref toCall = arena->external(S("create"), address(&createSimpleFloat));

	Listing *l = new (e) Listing();
	l->result = intDesc(e);
	Var v = l->createVar(l->root(), Size::sInt * 3);

	*l << prolog();

	*l << fnCall(toCall, retFloatDesc(e), v);
	*l << mov(eax, intRel(v, Offset()));
	*l << add(eax, intRel(v, Offset::sInt));

	*l << fnRet(eax);

	Binary *b = new (e) Binary(arena, l);
	typedef Int (*Fn)();
	Fn fn = (Fn)b->address();

	(*fn)();
	CHECK_EQ((*fn)(), 30);
} END_TEST
