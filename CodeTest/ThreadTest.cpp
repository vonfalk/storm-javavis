#include "stdafx.h"
#include "Test/Test.h"
#include "Code/UThread.h"
#include "Code/Debug.h"
#include "Utils/Thread.h"
#include "Utils/Exception.h"
#include "Tracker.h"

static int var = 0;
static THREAD int local = 0;

static void otherThread(Thread::Control &c) {
	var++;
	local++;
	assert(c.thread().sameAsCurrent());
	Sleep(10);
	var++;
	local++;
	assert(local == 2);
}

BEGIN_TEST(ThreadTest) {
	var = 0;
	local = 1;
	{
		Thread t;
		t.start(simpleFn(otherThread));
		t.stopWait();
		CHECK_EQ(var, 2);

		t.start(simpleFn(otherThread));
	}
	CHECK_EQ(var, 4);
	CHECK_EQ(local, 1);

} END_TEST

static int count1 = 0;
static int count2 = 0;

static void utFn1() {
	count1 = 1;
	UThread::leave();
	count1 = 10;
	UThread::leave();
	count1 = 4;
}

static void utFn2() {
	count2 = 1;
	UThread::leave();
	count2 = 10;
	UThread::leave();
	count2 = 4;
}

BEGIN_TEST(UThreadTest) {
	count1 = 0;
	count2 = 0;

	UThread::spawn(simpleVoidFn(&utFn1));
	CHECK_EQ(count1, 0);
	CHECK_EQ(count2, 0);
	UThread::leave();
	UThread::spawn(simpleVoidFn(&utFn2));
	CHECK_EQ(count1, 1);
	CHECK_EQ(count2, 0);
	UThread::leave();
	CHECK_EQ(count1, 10);
	CHECK_EQ(count2, 1);
	UThread::leave();
	CHECK_EQ(count1, 4);
	CHECK_EQ(count2, 10);
	UThread::leave();
	CHECK_EQ(count1, 4);
	CHECK_EQ(count2, 4);
	CHECK(!UThread::any())
	UThread::leave();
	CHECK_EQ(count1, 4);
	CHECK_EQ(count2, 4);

} END_TEST

static nat exceptions = 0;

static void exInner(nat depth = 0) {
	UThread::leave();
	if (depth == 3) {
		// dumpStack();
		throw UserError(L"Testing!");
	} else {
		exInner(depth + 1);
	}
}

static void exRoot() {
	try {
		exInner();
	} catch (const UserError &e) {
		exceptions += e.what() == L"Testing!";
	}
}

BEGIN_TEST(UThreadExTest) {
	exceptions = 0;

	UThread::spawn(simpleVoidFn(&exRoot));
	UThread::spawn(simpleVoidFn(&exRoot));

	// dumpStack();

	for (nat i = 0; i < 50; i++)
		UThread::leave();

	CHECK_EQ(exceptions, 2);
} END_TEST

// Test functions.
static nat fnCallSum = 0;

static void natNatFn(nat a, nat b) {
	fnCallSum += a + b;
}

static void trackerFn(Tracker a) {
	fnCallSum += a.data;
}

BEGIN_TEST(UThreadFnCallTest) {
	fnCallSum = 0;

	nat a = 10, b = 20;
	UThread::spawn(&natNatFn, FnCall().param(a).param(b));
	UThread::leave();
	CHECK_EQ(fnCallSum, 30);

	Tracker::clear();
	{
		Tracker t(12);
		UThread::spawn(&trackerFn, FnCall().param(t));
		UThread::leave();
	}
	CHECK_EQ(fnCallSum, 42);
	CHECK(Tracker::clear());

} END_TEST
