#include "stdafx.h"
#include "Code/Listing.h"
#include "Code/Binary.h"
#include "Code/Exception.h"

#include "Code/X64/Params.h"

using namespace code;

BEGIN_TEST(CodeScopeTest, CodeBasic) {
	Engine &e = gEngine();

	Listing *l = new (e) Listing();

	Part p = l->createPart(l->root());
	Block b = l->createBlock(l->root());
	Var v0 = l->createVar(l->root(), Size::sLong);
	Var v1 = l->createVar(b, Size::sInt);
	Var v2 = l->createVar(p, Size::sInt);
	Var v3 = l->createVar(p, Size::sInt);
	Var par = l->createPtrParam();

	*l << mov(eax, ebx);
	*l << mov(v2, intConst(10));

	CHECK_EQ(l->prev(v1), v3);
	CHECK_EQ(l->prev(v3), v2);
	CHECK_EQ(l->prev(v2), v0);
	CHECK_EQ(l->prev(v0), par);
	CHECK_EQ(l->prev(par), Var());

} END_TEST

BEGIN_TEST(CodeScopeTest2, CodeBasic) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);

	Listing *l = new (e) Listing();

	Block b0 = l->createBlock(l->root());
	Var v0 = l->createVar(b0, Size::sLong);
	Part p1 = l->createPart(b0);
	Var v1 = l->createVar(p1, Size::sLong);
	Part p2 = l->createPart(l->root());
	Var v2 = l->createVar(p2, Size::sInt);
	Block b3 = l->createBlock(p2);
	Var v3 = l->createVar(b3, Size::sInt);

	CHECK_EQ(l->prev(v0), v2);
	CHECK_EQ(l->prev(v1), v0);
	CHECK_EQ(l->prev(v2), Var());
	CHECK_EQ(l->prev(v3), v2);
	CHECK_EQ(l->prev(b0), l->root());
	CHECK_EQ(l->prev(p1), b0);
	CHECK_EQ(l->prev(p2), l->root());
	CHECK_EQ(l->prev(b3), p2);

	CHECK_EQ(l->parent(v0), b0);
	CHECK_EQ(l->parent(v1), p1);
	CHECK_EQ(l->parent(v2), p2);
	CHECK_EQ(l->parent(b0), l->root());
	CHECK_EQ(l->parent(p1), l->root());
	CHECK_EQ(l->parent(p2), Part());
	CHECK_EQ(l->parent(b3), p2);

	*l << prolog();
	*l << begin(b0);
	*l << begin(p1);
	*l << end(b0);
	*l << begin(p2);
	*l << begin(b3);
	*l << end(b3);
	*l << epilog();

	CHECK_RUNS(new (e) Binary(arena, l));

} END_TEST

BEGIN_TEST(CodeScopeTest3, CodeBasic) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);

	Listing *l = new (e) Listing();

	Block b1 = l->createBlock(l->root());
	Part p2 = l->createPart(b1);

	Var v0 = l->createVar(p2, Size::sLong);

	CHECK_EQ(l->parent(v0), p2);
	CHECK_EQ(l->parent(b1), l->root());
	CHECK_EQ(l->parent(p2), l->root());

	*l << prolog();
	*l << begin(b1);
	*l << lea(ptrA, v0);
	*l << end(b1);
	*l << begin(p2); // Should not be allowed!
	*l << lea(ptrA, v0);
	*l << epilog();

	CHECK_ERROR(new (e) Binary(arena, l), code::BlockBeginError);

} END_TEST

BEGIN_TEST(CodeTest, CodeBasic) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);

	Listing *l = new (e) Listing();

	Part root = l->root();
	Var v = l->createIntVar(root);
	Var p = l->createIntParam();

	Label l1 = l->label();

	CHECK_EQ(l->exceptionHandler(), false);

	*l << prolog();
	*l << mov(v, p);
	*l << add(v, intConst(1));
	*l << l1 << mov(ptrA, l1);
	// Use 'ebx' so that we have to preserve some registers during the function call...
	*l << mov(ebx, v);
	*l << mov(eax, ebx);
	*l << epilog();
	*l << ret(ValType(Size::sInt, false));

	Binary *b = new (e) Binary(arena, l);

	typedef Int (*Fn)(Int);
	Fn fn = (Fn)b->address();
	Int r = (*fn)(10);
	CHECK_EQ(r, 11);
} END_TEST


// Do some heavy GC allocations to make the Gc do a collection.
static void triggerCollect() {
	for (nat j = 0; j < 10; j++) {
		for (nat i = 0; i < 100; i++)
			new (gEngine()) Str(S("Hello"));

		gEngine().gc.collect();
	}
}

// Make sure the Gc are not moving code that is referred to on the stack, even if we may have
// unaligned pointers into blocks.
BEGIN_TEST(CodeGcTest, CodeBasic) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);

	Listing *l = new (e) Listing();

	*l << prolog();
	*l << call(arena->external(S("triggerCollect"), address(&triggerCollect)), valVoid());
	*l << mov(eax, intConst(1337));
	*l << epilog();
	*l << ret(ValType(Size::sInt, false));

	Binary *b = new (e) Binary(code::arena(e), l);

	typedef Int (*Fn)();
	Fn fn = (Fn)b->address();
	CHECK_EQ((*fn)(), 1337);
} END_TEST

BEGIN_TEST(CodeHereTest, CodeBasic) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);

	Listing *l = new (e) Listing();

	*l << prolog();
	*l << push(arena->external(S("triggerCollect"), address(&triggerCollect)));
	*l << pop(ptrA);
	*l << epilog();
	*l << ret(ValType(Size::sInt, false));

	Binary *b = new (e) Binary(code::arena(e), l);

	typedef void *(*Fn)();
	Fn fn = (Fn)b->address();
	CHECK_EQ((*fn)(), address(&triggerCollect));
} END_TEST

BEGIN_TEST_(CodeX64Layout, CodeBasic) {
	Engine &e = gEngine();

	code::x64::Params *p = new (e) code::x64::Params();
	p->add(0, new (e) PrimitiveDesc(intPrimitive()));
	p->add(1, new (e) PrimitiveDesc(floatPrimitive()));

	PVAR(p);
} END_TEST
