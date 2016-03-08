#include "stdafx.h"
#include "Test/Test.h"
#include "Fn.h"

BEGIN_TEST(SyntaxTest) {

	CHECK_RUNS(runFn<int>(L"test.syntax.testSentence"));
	CHECK_RUNS(runFn<int>(L"test.syntax.testArray"));
	CHECK_RUNS(runFn<int>(L"test.syntax.testEmpty"));

} END_TEST
