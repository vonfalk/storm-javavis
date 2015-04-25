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
	CHECK_EQ(runFn(L"test.bs.actorDerObj"), 22);

	// Future tests.
	CHECK_EQ(runFn(L"test.bs.basicFuture"), 8);
	CHECK_EQ(runFn(L"test.bs.valueFuture"), 8);
	CHECK_EQ(runFn(L"test.bs.intFuture"), 22);

	DbgVal::clear();
	CHECK_RUNS(runFn(L"test.bs.noResultFuture"));
	CHECK(DbgVal::clear());

	// Check 'spawn'.
	DbgVal::clear();
	CHECK_EQ(runFn(L"test.bs.asyncDbgVal"), 18);
	CHECK_EQ(runFn(L"test.bs.asyncDbgVal2"), 18);
	CHECK(DbgVal::clear());
	CHECK_EQ(runFn(L"test.bs.unusedDbgVal"), 33);
	Sleep(100);
	CHECK(DbgVal::clear());
	CHECK_EQ(runFn(L"test.bs.asyncObject"), 15);
	CHECK_RUNS(runFn(L"test.bs.spawnVoid"));

} END_TEST
