#include "stdafx.h"
#include "Test/Test.h"
#include "Fn.h"
#include "Storm/Lib/FnPtr.h"

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
	Auto<Str> original = CREATE(Str, e, L"A");
	Auto<Str> copy = strFn->call(original);
	CHECK(original.borrow() != copy.borrow());

	Auto<FnPtr<DbgVal, Int>> dbgValFn = fnPtr(e, &returnDbgVal);
	CHECK_EQ(dbgValFn->call(23), DbgVal(23));

	Auto<FnPtr<Dbg *, Int>> dbgFn = fnPtr(e, &returnDbg);
	CHECK_EQ(steal(dbgFn->call(20))->get(), 20);

	Auto<FnPtr<Str *, Par<Str>>> echoStr = fnPtr(e, &::echoStr);
	Auto<FnPtr<void, Par<Str>>> consumeStr = fnPtr(e, &::consumeStr);

	Auto<FnPtr<Int, Par<Dbg>>> consumeDbg = fnPtr(e, &::fromDbg);
	CHECK_EQ(consumeDbg->call(steal(CREATE(Dbg, e, 2))), 2);

	// Run some code in Storm!
	CHECK_RUNS(runFnInt(L"test.bs.consumeStr", consumeStr));
	CHECK_EQ(runFnInt(L"test.bs.consumeDbg", consumeDbg), 23);
	CHECK_EQ(runFnInt(L"test.bs.runFnPtr", fn), 12);
	CHECK_EQ(runFnInt(L"test.bs.runFnPtr", voidFn), 5);
	CHECK_EQ(runFnInt(L"test.bs.runStrFnPtr", strFn), 22);
	CHECK_EQ(runFnInt(L"test.bs.runStrFnPtr", echoStr), 22);
	CHECK_EQ(runFnInt(L"test.bs.runFnPtr", dbgValFn), 23);
	CHECK_EQ(runFnInt(L"test.bs.runFnPtr", dbgFn), 21);

	TODO(L"Verify that clone works.");
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
