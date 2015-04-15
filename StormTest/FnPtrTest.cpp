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
	PLN("Created dbg object.");
	return CREATE(Dbg, *gEngine, v);
}

static Int fromDbg(Par<Dbg> dbg) {
	return dbg->get();
}

static Str *echoStr(Par<Str> v) {
	return v.ret();
}

static void consumeStr(Par<Str> v) {
	PVAR(v);
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
