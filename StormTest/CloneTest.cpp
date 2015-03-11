#include "stdafx.h"
#include "Test/Test.h"
#include "Fn.h"
#include "Storm/Lib/CloneEnv.h"
#include "Storm/Lib/Debug.h"

BEGIN_TEST(CloneTest) {

	// Auto<Object> o = runFn<Object*>(L"test.bs.createBase");
	// Auto<CloneEnv> e = CREATE(CloneEnv, *gEngine);
	// PLN("Calling!");
	// o->deepCopy(e);

	CHECK_EQ(runFn(L"test.bs.testClone"), 1);

} END_TEST
