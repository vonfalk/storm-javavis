#include "stdafx.h"
#include "Test/Lib/Test.h"
#include "Fn.h"
#include "Shared/FnPtr.h"

static Int addTwo(Int v) {
	return v + 2;
}

static Int returnFive() {
	return 5;
}

static DbgVal returnDbgVal(Int v) {
	return DbgVal(v);
}

static Dbg *returnDbg(Int v) {
	return CREATE(Dbg, *gEngine, v);
}

static Int fromDbg(Par<Dbg> dbg) {
	return dbg->get();
}

static Str *echoStr(Par<Str> v) {
	return v.ret();
}

static Bool bool1() {
	return true;
}

static Bool bool2() {
	return false;
}

static void consumeStr(Par<Str> v) {}

static Auto<FnPtr<DbgVal, Par<Dbg>>> dbgFn(int id) {
	typedef FnPtr<DbgVal, Par<Dbg>> Fn;
	return runFn<Fn *>(L"test.bs.createDbgFn", id);
}

static Auto<FnPtr<DbgVal>> voidFn(int id) {
	typedef FnPtr<DbgVal> Fn;
	return runFn<Fn *>(L"test.bs.createVoidFn", id);
}

static Auto<FnPtr<DbgVal, Par<DbgActor>>> actorFn(int id) {
	typedef FnPtr<DbgVal, Par<DbgActor>> Fn;
	return runFn<Fn *>(L"test.bs.createActorFn", id);
}

BEGIN_TEST(FnPtrTest) {
	Engine &e = *gEngine;

	Auto<FnPtr<Int, Int>> fn = fnPtr(e, &addTwo);
	CHECK_EQ(fn->call(10), 12);

	Auto<FnPtr<Int>> voidFn = fnPtr(e, &returnFive);
	CHECK_EQ(voidFn->call(), 5);

	Auto<DbgActor> actor = CREATE(DbgActor, e);
	Auto<FnPtr<Str *, Par<Str>>> strFn = memberWeakPtr(e, actor, &DbgActor::echo);

	Auto<FnPtr<DbgVal, Int>> dbgValFn = fnPtr(e, &returnDbgVal);
	CHECK_EQ(dbgValFn->call(23), DbgVal(23));

	Auto<FnPtr<Dbg *, Int>> dbgFn = fnPtr(e, &returnDbg);
	CHECK_EQ(steal(dbgFn->call(20))->get(), 20);

	Auto<FnPtr<Str *, Par<Str>>> echoStr = fnPtr(e, &::echoStr);
	Auto<FnPtr<void, Par<Str>>> consumeStr = fnPtr(e, &::consumeStr);

	Auto<FnPtr<Int, Par<Dbg>>> consumeDbg = fnPtr(e, &::fromDbg);
	CHECK_EQ(consumeDbg->call(steal(CREATE(Dbg, e, 2))), 2);

	Auto<FnPtr<Bool>> b1 = fnPtr(e, &::bool1);
	Auto<FnPtr<Bool>> b2 = fnPtr(e, &::bool2);
	CHECK_EQ(b1->call(), true);
	CHECK_EQ(b2->call(), false);

	// Run some code in Storm!
	CHECK_RUNS(runFnInt(L"test.bs.consumeStr", consumeStr));
	CHECK_EQ(runFnInt(L"test.bs.consumeDbg", consumeDbg), 23);
	CHECK_EQ(runFnInt(L"test.bs.runFnPtr", fn), 12);
	CHECK_EQ(runFnInt(L"test.bs.runFnPtr", voidFn), 5);
	CHECK_EQ(runFnInt(L"test.bs.runStrFnPtr", strFn), 22);
	CHECK_EQ(runFnInt(L"test.bs.runStrFnPtr", echoStr), 22);
	CHECK_EQ(runFnInt(L"test.bs.runFnPtr", dbgValFn), 23);
	CHECK_EQ(runFnInt(L"test.bs.runFnPtr", dbgFn), 21);
	CHECK_EQ(runFnInt(L"test.bs.runBoolFn", b1), 1);
	CHECK_EQ(runFnInt(L"test.bs.runBoolFn", b2), 0);

	b1 = runFn<FnPtr<Bool> *>(L"test.bs.createBoolFn", true);
	b2 = runFn<FnPtr<Bool> *>(L"test.bs.createBoolFn", false);
	CHECK_EQ(b1->call(), true);
	CHECK_EQ(b2->call(), false);
	CHECK_EQ(runFnInt(L"test.bs.runBoolFn", b1), 1);
	CHECK_EQ(runFnInt(L"test.bs.runBoolFn", b2), 0);

} END_TEST


BEGIN_TEST(StormFnPtrTest) {
	Engine &e = *gEngine;

	// Return a function pointer from Storm!
	{
		typedef FnPtr<Int, Int> IIFn;
		Auto<IIFn> v = runFn<IIFn *>(L"test.bs.createIntFn");
		CHECK_EQ(v->call(10), 20);
	}


	{
		Auto<Dbg> dbg = CREATE(Dbg, e, 5);
		CHECK_EQ(dbgFn(0)->call(dbg), DbgVal(15));
		CHECK_EQ(dbgFn(1)->call(dbg), DbgVal(25));
	}

	{
		CHECK_EQ(voidFn(0)->call(), DbgVal(20));
		CHECK_EQ(voidFn(1)->call(), DbgVal(12));
		CHECK_EQ(voidFn(2)->call(), DbgVal(22));
	}

	// Actors.
	{
		Auto<DbgActor> actor = CREATE(DbgActor, e, 7);
		CHECK_EQ(actorFn(0)->call(actor), DbgVal(27));
		CHECK_EQ(actorFn(1)->call(actor), DbgVal(11));
	}

} END_TEST

// Run the function pointer both through Storm and C++ and compare the results.
static int runPtr(Par<FnPtr<Int, Par<Dbg>>> ptr, int start) {
	Auto<Dbg> dbg = runFn<Dbg *>(L"test.bs.dbgTrack", start);
	int storm = runFnInt(L"test.bs.runPtr", ptr, dbg);
	int cpp = ptr->call(dbg);
	if (storm != cpp) {
		PLN("Storm and C++ gives different results!");
		return -1;
	}
	return storm;
}

// Run the function pointer through another thread (only Storm) and see the result.
static int runPtrOther(Par<FnPtr<Int, Par<Dbg>>> ptr, int start) {
	Auto<Dbg> dbg = runFn<Dbg *>(L"test.bs.dbgTrack", start);
	return runFnInt(L"test.bs.runPtrOther", ptr, dbg);
}


static Auto<FnPtr<Int, Par<Dbg>>> dbgPtr(int id) {
	return runFn<FnPtr<Int, Par<Dbg>> *>(L"test.bs.trackParam", id);
}

BEGIN_TEST(FnPtrThreadTest) {
	Engine &e = *gEngine;

	CHECK_EQ(runPtr(dbgPtr(0), 1), 1);
	CHECK_EQ(runPtr(dbgPtr(1), 1), 10101);
	CHECK_EQ(runPtr(dbgPtr(2), 1), 1);
	CHECK_EQ(runPtr(dbgPtr(3), 1), 10101);
	CHECK_EQ(runPtr(dbgPtr(4), 1), 1);

	// Note: these are always copied once because we're switching to another thread before calling the
	// function pointer.
	CHECK_EQ(runPtrOther(dbgPtr(0), 1), 10101);
	CHECK_EQ(runPtrOther(dbgPtr(1), 1), 10101);
	CHECK_EQ(runPtrOther(dbgPtr(2), 1), 20201);
	CHECK_EQ(runPtrOther(dbgPtr(3), 1), 10101);
	CHECK_EQ(runPtrOther(dbgPtr(4), 1), 20201);

	Auto<FnPtr<Int, Par<Dbg>>> fromDbg = fnPtr(e, &::fromDbg);
	CHECK_EQ(runPtrOther(dbgPtr(0), 1), 10101);

	// Actor as the first parameter...
	{
		Auto<Dbg> dbg = runFn<Dbg *>(L"test.bs.dbgTrack", 1);
		Auto<TObject> dbgActor = runFn<TObject *>(L"test.bs.createInActor");
		Auto<FnPtr<Int, Par<TObject>, Par<Dbg>>> fn =
			runFn<FnPtr<Int, Par<TObject>, Par<Dbg>> *>(L"test.bs.trackActor");
		int storm = runFnInt(L"test.bs.runPtrActor", fn, dbg);
		int cpp = fn->call(dbgActor, dbg);
		CHECK_EQ(storm, 10101);
		CHECK_EQ(cpp, 10101);
	}

	// Return values.
	{
		Auto<FnPtr<Dbg *>> fn = runFn<FnPtr<Dbg *> *>(L"test.bs.dbgFnPtr");

		Auto<Dbg> cpp = fn->call();
		CHECK_EQ(cpp->get(), 10101);

		int storm = runFnInt(L"test.bs.callDbgFnPtr", fn.borrow());
		CHECK_EQ(storm, 10101);
	}

} END_TEST
