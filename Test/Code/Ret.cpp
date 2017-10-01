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

BEGIN_TEST(RetComplex, Code) {
	using storm::debug::DbgVal;
	Engine &e = gEngine();
	Arena *arena = code::arena(e);
	Type *dbgVal = DbgVal::stormType(e);

	Listing *l = new (e) Listing();
	l->result = new (e) ComplexDesc(dbgVal->size(), dbgVal->copyCtor()->ref(), dbgVal->destructor()->ref());
	Var v = l->createVar(l->root(), dbgVal->size());

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

BEGIN_TEST(RetRefComplex, Code) {
	using storm::debug::DbgVal;
	Engine &e = gEngine();
	Arena *arena = code::arena(e);
	Type *dbgVal = DbgVal::stormType(e);

	Listing *l = new (e) Listing();
	l->result = new (e) ComplexDesc(dbgVal->size(), dbgVal->copyCtor()->ref(), dbgVal->destructor()->ref());
	Var v = l->createVar(l->root(), dbgVal->size());

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
