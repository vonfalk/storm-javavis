#include "stdafx.h"
#include "Fn.h"
#include "Compiler/Debug.h"

BEGIN_TEST_(BSThread, BS) {
	using namespace storm::debug;

	Engine &e = gEngine();

	Thread *thread = runFn<Thread *>(L"test.bs.getThread");
	CHECK_NEQ(thread, storm::Compiler::thread(e));
	thread = runFn<Thread *>(L"test.bs.getCompilerThread");
	CHECK_EQ(thread, storm::Compiler::thread(e));

	CHECK_EQ(runFn<Int>(L"test.bs.postInt"), 9);

	// Check so that we keep reference counting correct.
	DbgVal::clear();
	CHECK_EQ(runFn<Int>(L"test.bs.postDbgVal"), 18);
	CHECK(DbgVal::clear());

	// Basic thread tests.
	CHECK_EQ(runFn<Int>(L"test.bs.postObject"), 13);
	CHECK_EQ(runFn<Int>(L"test.bs.postVal"), 33);
	CHECK_EQ(runFn<Int>(L"test.bs.threadObj"), 20);
	CHECK_EQ(runFn<Int>(L"test.bs.threadActor"), 20);
	CHECK_EQ(runFn<Int>(L"test.bs.actorObj"), 31);
	CHECK_EQ(runFn<Int>(L"test.bs.actorDerObj"), 22);

	// Future tests.
	CHECK_EQ(runFn<Int>(L"test.bs.basicFuture"), 8);
	CHECK_EQ(runFn<Int>(L"test.bs.valueFuture"), 8);
	CHECK_EQ(runFn<Int>(L"test.bs.intFuture"), 22);
	CHECK_RUNS(runFn<void>(L"test.bs.noResultFuture"));

	// Check 'spawn'.
	CHECK_EQ(runFn<Int>(L"test.bs.asyncPostInt"), 9);
	CHECK_EQ(runFn<Int>(L"test.bs.asyncPostObject"), 13);
	CHECK_EQ(runFn<Int>(L"test.bs.asyncPostObject2"), 13);
	CHECK_EQ(runFn<Int>(L"test.bs.asyncPostVal"), 33);
	CHECK_RUNS(runFn<void>(L"test.bs.spawnVoid"));

} END_TEST
