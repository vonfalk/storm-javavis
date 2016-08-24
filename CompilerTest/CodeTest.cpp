#include "stdafx.h"
#include "Test/Test.h"
#include "Code/Listing.h"

using namespace code;

BEGIN_TEST(CodeTest, Code) {
	Engine &e = *gEngine;

	Listing *l = new (e) Listing();

	Part p = l->createPart(l->root());
	Block b = l->createBlock(l->root());
	l->createVar(l->root(), Size::sLong);
	l->createVar(b, Size::sInt);
	l->createVar(p, Size::sInt);

	*l << mov(e, eax, ebx);

	PVAR(l);

} END_TEST
