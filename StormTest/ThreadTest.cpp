#include "stdafx.h"
#include "Test/Test.h"
#include "Fn.h"
#include "Storm/Thread.h"

BEGIN_TEST(ThreadTest) {

	Auto<Thread> thread = runFn<Thread*>(L"test.bs.getThread");
	CHECK_NEQ(thread.borrow(), storm::Compiler.thread(*gEngine));
	thread = runFn<Thread*>(L"test.bs.getCompilerThread");
	CHECK_EQ(thread.borrow(), storm::Compiler.thread(*gEngine));

	CHECK_EQ(runFn(L"test.bs.postInt"), 9);

} END_TEST
