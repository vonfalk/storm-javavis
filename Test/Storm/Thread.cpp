#include "stdafx.h"
#include "Fn.h"
#include "Compiler/Debug.h"
#include "Compiler/Exception.h"

BEGIN_TEST(BSThread, BS) {
	using namespace storm::debug;

	Engine &e = gEngine();

	Thread *thread = runFn<Thread *>(S("test.bs.getThread"));
	CHECK_NEQ(thread, storm::Compiler::thread(e));
	thread = runFn<Thread *>(S("test.bs.getCompilerThread"));
	CHECK_EQ(thread, storm::Compiler::thread(e));

	CHECK_EQ(runFn<Int>(S("test.bs.postInt")), 9);

	// Check so that we keep reference counting correct.
	DbgVal::clear();
	CHECK_EQ(runFn<Int>(S("test.bs.postDbgVal")), 18);
	CHECK(DbgVal::clear());

	// Basic thread tests.
	CHECK_EQ(runFn<Int>(S("test.bs.postObject")), 13);
	CHECK_EQ(runFn<Int>(S("test.bs.postVal")), 33);
	CHECK_EQ(runFn<Int>(S("test.bs.threadObj")), 20);
	CHECK_EQ(runFn<Int>(S("test.bs.threadActor")), 20);
	CHECK_EQ(runFn<Int>(S("test.bs.actorObj")), 31);
	CHECK_EQ(runFn<Int>(S("test.bs.actorDerObj")), 22);

	// Future tests.
	CHECK_EQ(runFn<Int>(S("test.bs.basicFuture")), 8);
	CHECK_EQ(runFn<Int>(S("test.bs.valueFuture")), 8);
	CHECK_EQ(runFn<Int>(S("test.bs.intFuture")), 22);
	CHECK_RUNS(runFn<void>(S("test.bs.noResultFuture")));

	// Check 'spawn'.
	CHECK_EQ(runFn<Int>(S("test.bs.asyncPostInt")), 9);
	CHECK_EQ(runFn<Int>(S("test.bs.asyncPostObject")), 13);
	CHECK_EQ(runFn<Int>(S("test.bs.asyncPostObject2")), 13);
	CHECK_EQ(runFn<Int>(S("test.bs.asyncPostVal")), 33);
	CHECK_RUNS(runFn<void>(S("test.bs.spawnVoid")));

	// Check variable accesses in other threads.
	CHECK_EQ(runFn<Int>(S("test.bs.threadVarAccess")), 6); // 1 copy, 1 deep copy. Starts at 4.
	CHECK_ERROR(runFn<void>(S("test.bs.threadVarAssign")), SyntaxError);
} END_TEST
