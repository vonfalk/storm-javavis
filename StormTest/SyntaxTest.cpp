#include "stdafx.h"
#include "Test/Test.h"
#include "Fn.h"

BEGIN_TEST_(SyntaxTest) {

	// CHECK_RUNS(runFn<int>(L"test.syntax.testSimple"));
	// CHECK_RUNS(runFn<int>(L"test.syntax.testSentence"));
	// CHECK_RUNS(runFn<int>(L"test.syntax.testMaybe"));
	// CHECK_RUNS(runFn<int>(L"test.syntax.testArray"));
	CHECK_RUNS(runFn<int>(L"test.syntax.testCall"));
	// CHECK_RUNS(runFn<int>(L"test.syntax.testEmpty"));

	// TODO: Check so that parameters work properly!

} END_TEST
