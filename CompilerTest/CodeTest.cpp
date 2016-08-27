#include "stdafx.h"
#include "Test/Test.h"
#include "Code/Listing.h"

using namespace code;

BEGIN_TEST(CodeTest, Code) {
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

	PVAR(l);

	CHECK_EQ(l->prev(v1), v3);
	CHECK_EQ(l->prev(v3), v2);
	CHECK_EQ(l->prev(v2), v0);
	CHECK_EQ(l->prev(v0), par);
	CHECK_EQ(l->prev(par), Variable());

} END_TEST
