#include "stdafx.h"
#include "Test/Test.h"
#include "Fn.h"
#include "Storm/Thread.h"

BEGIN_TEST(ThreadTest) {

	Auto<Thread> thread = runFn<Thread*>(L"test.bs.getThread");
	CHECK_NEQ(thread.borrow(), storm::Compiler::thread(*gEngine));
	thread = runFn<Thread*>(L"test.bs.getCompilerThread");
	CHECK_EQ(thread.borrow(), storm::Compiler::thread(*gEngine));

	CHECK_EQ(runFn(L"test.bs.postInt"), 9);

	// Check so that we keep reference counting correct.
	DbgVal::clear();
	CHECK_EQ(runFn(L"test.bs.postDbgVal"), 18);
	CHECK(DbgVal::clear());

	CHECK_EQ(runFn(L"test.bs.postObject"), 13);
	CHECK_EQ(runFn(L"test.bs.postVal"), 33);
	CHECK_EQ(runFn(L"test.bs.threadObj"), 20);
	CHECK_EQ(runFn(L"test.bs.threadActor"), 20);
	CHECK_EQ(runFn(L"test.bs.actorObj"), 31);

	// TODO: Implement som async/fork test here as well!
} END_TEST
