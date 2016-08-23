#include "stdafx.h"
#include "Test/Test.h"
#include "Code/Listing.h"

using namespace code;

BEGIN_TEST(CodeTest, Code) {
	Engine &e = *gEngine;

	Listing *l = new (e) Listing();
	*l << mov(e, eax, ebx);

	PVAR(l);

} END_TEST
