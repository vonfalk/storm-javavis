#include "stdafx.h"
#include "Fn.h"
#include "Compiler/Debug.h"

BEGIN_TEST(BSThread, BS) {
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

	CHECK_EQ(runFn<Int>(L"test.bs.postObject"), 13);
	CHECK_EQ(runFn<Int>(L"test.bs.postVal"), 33);
	CHECK_EQ(runFn<Int>(L"test.bs.threadObj"), 20);
	CHECK_EQ(runFn<Int>(L"test.bs.threadActor"), 20);
	CHECK_EQ(runFn<Int>(L"test.bs.actorObj"), 31);
	CHECK_EQ(runFn<Int>(L"test.bs.actorDerObj"), 22);

	// // Future tests.
	// CHECK_EQ(runFn<Int>(L"test.bs.basicFuture"), 8);
	// CHECK_EQ(runFn<Int>(L"test.bs.valueFuture"), 8);
	// CHECK_EQ(runFn<Int>(L"test.bs.intFuture"), 22);

	// DbgVal::clear();
	// CHECK_RUNS(runFn<Int>(L"test.bs.noResultFuture"));
	// CHECK(DbgVal::clear());

	// // Check 'spawn'.
	// DbgVal::clear();
	// CHECK_EQ(runFn<Int>(L"test.bs.asyncDbgVal"), 18);
	// CHECK_EQ(runFn<Int>(L"test.bs.asyncDbgVal2"), 18);
	// CHECK(DbgVal::clear());
	// CHECK_EQ(runFn<Int>(L"test.bs.unusedDbgVal"), 33);
	// Sleep(100);
	// CHECK(DbgVal::clear());
	// CHECK_EQ(runFn<Int>(L"test.bs.asyncObject"), 15);
	// CHECK_RUNS(runFn<Int>(L"test.bs.spawnVoid"));
} END_TEST
