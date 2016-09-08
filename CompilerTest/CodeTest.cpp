#include "stdafx.h"
#include "Test/Test.h"
#include "Code/Listing.h"
#include "Code/Binary.h"

using namespace code;

BEGIN_TEST(CodeScopeTest, Code) {
	Engine &e = *gEngine;

	Listing *l = new (e) Listing();

	Part p = l->createPart(l->root());
	Block b = l->createBlock(l->root());
	Variable v0 = l->createVar(l->root(), Size::sLong);
	Variable v1 = l->createVar(b, Size::sInt);
	Variable v2 = l->createVar(p, Size::sInt);
	Variable v3 = l->createVar(p, Size::sInt);
	Variable par = l->createParam(valPtr());

	*l << mov(e, eax, ebx);
	*l << mov(e, v2, intConst(10));

	CHECK_EQ(l->prev(v1), v3);
	CHECK_EQ(l->prev(v3), v2);
	CHECK_EQ(l->prev(v2), v0);
	CHECK_EQ(l->prev(v0), par);
	CHECK_EQ(l->prev(par), Variable());

} END_TEST

BEGIN_TEST(CodeTest, Code) {
	Engine &e = *gEngine;
	Arena *arena = code::arena(e);

	Listing *l = new (e) Listing();

	Part root = l->root();
	Variable v = l->createIntVar(root);
	Variable p = l->createIntParam();

	CHECK_EQ(l->exceptionHandler(), false);

	*l << prolog(e);
	*l << mov(e, v, p);
	*l << add(e, v, intConst(1));
	// Use 'ebx' so that we have to preserve some registers during the function call...
	*l << mov(e, ebx, v);
	*l << mov(e, eax, ebx);
	*l << epilog(e);
	*l << ret(e, ValType(Size::sInt, false));

	PVAR(l);

	Binary *b = new (e) Binary(arena, l);

	typedef Int (*Fn)(Int);
	Fn fn = (Fn)b->rawPtr();
	Int r = (*fn)(10);
	CHECK_EQ(r, 11);
} END_TEST
