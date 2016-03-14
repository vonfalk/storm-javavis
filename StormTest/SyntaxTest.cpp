#include "stdafx.h"
#include "Test/Test.h"
#include "Fn.h"

BEGIN_TEST(SyntaxTest) {

	CHECK_RUNS(runFn<int>(L"test.syntax.testSimple"));
	CHECK(runFn<Bool>(L"test.syntax.testSentence"));
	CHECK(runFn<Bool>(L"test.syntax.testMaybe"));
	CHECK(runFn<Bool>(L"test.syntax.testArray"));
	CHECK(runFn<Bool>(L"test.syntax.testCall"));
	CHECK(runFn<Bool>(L"test.syntax.testCallMaybe"));
	CHECK_EQ(runFn<Nat>(L"test.syntax.testRaw"), 2);
	CHECK_EQ(runFn<Nat>(L"test.syntax.testRawCall"), 2);
	CHECK(runFn<Bool>(L"test.syntax.testCapture"));
	CHECK(runFn<Bool>(L"test.syntax.testRawCapture"));
	CHECK_RUNS(runFn<int>(L"test.syntax.testParams"));
	CHECK(runFn<Bool>(L"test.syntax.testEmpty"));
	CHECK_EQ(runFn<Int>(L"test.syntax.testExpr"), 40);

} END_TEST

BEGIN_TEST_(SynTest) {

	CHECK_RUNS(runFn<int>(L"test.syntax.testBS"));

} END_TEST
