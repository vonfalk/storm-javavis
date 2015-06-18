#include "stdafx.h"
#include "Test/Test.h"
#include "Fn.h"
#include "Shared/CloneEnv.h"
#include "Storm/Lib/Debug.h"

BEGIN_TEST(CloneTest) {

	CHECK(runFn<Bool>(L"test.bs.testClone"));
	CHECK(runFn<Bool>(L"test.bs.testCloneDerived"));
	CHECK(runFn<Bool>(L"test.bs.testCloneValue"));
	CHECK_EQ(runFn<Int>(L"test.bs.testCloneArray"), 10);

} END_TEST
