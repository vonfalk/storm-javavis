#include "stdafx.h"
#include "Test/Test.h"
#include "Fn.h"

BEGIN_TEST_(SyntaxTest) {

	CHECK_RUNS(runFn<int>(L"test.syntax.testSimple"));
	CHECK_RUNS(runFn<int>(L"test.syntax.testSentence"));
	CHECK_RUNS(runFn<int>(L"test.syntax.testMaybe"));
	CHECK_RUNS(runFn<int>(L"test.syntax.testArray"));
	CHECK_RUNS(runFn<int>(L"test.syntax.testCall"));
	CHECK_RUNS(runFn<int>(L"test.syntax.testCallMaybe"));
	CHECK_RUNS(runFn<int>(L"test.syntax.testRaw"));
	CHECK_RUNS(runFn<int>(L"test.syntax.testRawCall"));
	CHECK_RUNS(runFn<int>(L"test.syntax.testCapture"));
	CHECK_RUNS(runFn<int>(L"test.syntax.testRawCapture"));
	CHECK_RUNS(runFn<int>(L"test.syntax.testParams"));
	CHECK_RUNS(runFn<int>(L"test.syntax.testEmpty"));

} END_TEST
