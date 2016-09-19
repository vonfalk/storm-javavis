#include "stdafx.h"
#include "Test/Test.h"
#include "Code/Listing.h"
#include "Code/Binary.h"

using namespace code;

BEGIN_TEST(CodeScopeTest, CodeBasic) {
	Engine &e = *gEngine;

	Listing *l = new (e) Listing();

	Part p = l->createPart(l->root());
	Block b = l->createBlock(l->root());
	Variable v0 = l->createVar(l->root(), Size::sLong);
	Variable v1 = l->createVar(b, Size::sInt);
	Variable v2 = l->createVar(p, Size::sInt);
	Variable v3 = l->createVar(p, Size::sInt);
	Variable par = l->createParam(valPtr());

	*l << mov(eax, ebx);
	*l << mov(v2, intConst(10));

	CHECK_EQ(l->prev(v1), v3);
	CHECK_EQ(l->prev(v3), v2);
	CHECK_EQ(l->prev(v2), v0);
	CHECK_EQ(l->prev(v0), par);
	CHECK_EQ(l->prev(par), Variable());

} END_TEST

BEGIN_TEST(CodeTest, CodeBasic) {
	Engine &e = *gEngine;
	Arena *arena = code::arena(e);

	Listing *l = new (e) Listing();

	Part root = l->root();
	Variable v = l->createIntVar(root);
	Variable p = l->createIntParam();

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
			new (*gEngine) Str(L"Hello");

		gEngine->gc.collect();
	}
}

// Make sure the Gc are not moving code that is referred to on the stack, even if we may have
// unaligned pointers into blocks.
BEGIN_TEST(CodeGcTest, CodeBasic) {
	Engine &e = *gEngine;

	Listing *l = new (e) Listing();

	*l << prolog();
	*l << call(xConst(Size::sPtr, (Word)&triggerCollect), valVoid());
	*l << mov(eax, intConst(1337));
	*l << epilog();
	*l << ret(ValType(Size::sInt, false));

	Binary *b = new (e) Binary(code::arena(e), l);

	typedef Int (*Fn)();
	Fn fn = (Fn)b->address();
	CHECK_EQ((*fn)(), 1337);

} END_TEST
