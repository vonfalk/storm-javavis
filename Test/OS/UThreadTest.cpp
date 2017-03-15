#include "stdafx.h"
#include "OS/Thread.h"
#include "OS/UThread.h"
#include "OS/ThreadGroup.h"
#include "Tracker.h"

using namespace os;

static void returnVoid(bool error) {
	if (error)
		throw UserError(L"ERROR");
}

static void voidParams(int a, int b) {
	assert(a == 10);
	assert(b == 20);
}

struct Dummy {
	void CODECALL voidMember(int a, int b) {
		assert((int)this == 10);
		assert(a == 20);
		assert(b == 30);
	}
};

static int returnInt(int v) {
	return v;
}

static int64 returnInt64(int64 v) {
	return v;
}

static float returnFloat(float v) {
	return v;
}

static double returnDouble(double v) {
	return v;
}

static Tracker returnTracker(int t) {
	if (t < 0)
		throw UserError(L"ERROR");
	return Tracker(t);
}

static int takeTracker(Tracker t) {
	if (t.data < 0)
		throw UserError(L"ERROR");
	return t.data;
}

struct DummyTracker {
	Tracker t;
	DummyTracker(int v) : t(v) {}

	Tracker CODECALL value() {
		return t;
	}
};

BEGIN_TEST(UThreadResultTest, OS) {
	// If these tests start to fail, consider increasing the stack space for
	// calling functions defined in UThread.cpp (pushParams).

	{
		bool e = false;
		FnParams params; params.add(e);
		os::Future<void> r;
		UThread::spawn(returnVoid, false, params, r);
		CHECK_RUNS(r.result());

		e = true;
		os::Future<void> r2;
		UThread::spawn(returnVoid, false, params, r2);
		CHECK_ERROR(r2.result(), UserError);
	}

	{
		os::Future<void> r;
		int a = 10, b = 20;
		FnParams p; p.add(a).add(b);
		UThread::spawn(voidParams, false, p, r);
		CHECK_RUNS(r.result());
	}

	{
		os::Future<void> r;
		void *a = (void *)10;
		int b = 20;
		int c = 30;
		FnParams p; p.add(a).add(b).add(c);
		UThread::spawn(address(&Dummy::voidMember), true, p, r);
		CHECK_RUNS(r.result());
	}

	{
		int v = 10;
		FnParams params; params.add(v);
		os::Future<int> r;
		UThread::spawn(returnInt, false, params, r);
		CHECK_EQ(r.result(), 10);
	}

	{
		int64 v = 1024LL << 30LL;
		FnParams params; params.add(v);
		os::Future<int64> r;
		UThread::spawn(returnInt64, false, params, r);
		CHECK_EQ(r.result() >> 30LL, 1024);
	}

	{
		float v = 13.37f;
		FnParams params; params.add(v);
		os::Future<float> r;
		UThread::spawn(returnFloat, false, params, r);
		CHECK_EQ(r.result(), 13.37f);
	}

	{
		double v = 13.37;
		FnParams params; params.add(v);
		os::Future<double> r;
		UThread::spawn(returnDouble, false, params, r);
		CHECK_EQ(r.result(), 13.37);
	}

	Tracker::clear();
	{
		int t = 22;
		FnParams params; params.add(t);
		os::Future<Tracker> r;
		UThread::spawn(returnTracker, false, params, r);
		CHECK_EQ(r.result().data, 22);
		CHECK_EQ(r.result().data, 22);

		t = -2;
		os::Future<Tracker> r2;
		UThread::spawn(returnTracker, false, params, r2);
		CHECK_ERROR(r2.result(), UserError);
		CHECK_ERROR(r2.result(), UserError);
	}
	CHECK(Tracker::clear());

	Tracker::clear();
	{
		Tracker t(22);
		FnParams params; params.add(t);
		os::Future<int> r;
		UThread::spawn(takeTracker, false, params, r);
		CHECK_EQ(r.result(), 22);
		CHECK_EQ(r.result(), 22);

		t.data = -3;
		os::Future<int> r2;
		UThread::spawn(takeTracker, false, params, r2);
		CHECK_ERROR(r2.result(), UserError);
		CHECK_ERROR(r2.result(), UserError);
	}
	CHECK(Tracker::clear());

	Tracker::clear();
	{
		DummyTracker t(12);
		DummyTracker *pT = &t;
		FnParams params; params.add(pT);
		os::Future<Tracker> r;
		UThread::spawn(address(&DummyTracker::value), true, params, r);
		CHECK_EQ(r.result().data, 12);
	}
	CHECK(Tracker::clear());

} END_TEST


struct SemaTest {
	os::Sema sema;

	nat state;

	SemaTest() : sema(1), state(0) {}

	void run() {
		// Should not block.
		sema.down();

		state = 1;

		// Should block.
		sema.down();

		state = 2;
	}
};


BEGIN_TEST(UThreadSema, OS) {
	SemaTest t;
	UThread::spawn(util::memberVoidFn(&t, &SemaTest::run));

	CHECK_EQ(t.state, 0);
	UThread::leave();
	CHECK_EQ(t.state, 1);
	for (nat i = 0; i < 20; i++)
		UThread::leave();
	CHECK_EQ(t.state, 1);

	t.sema.up();
	UThread::leave();
	CHECK_EQ(t.state, 2);

} END_TEST

struct SemaInterop {
	os::Sema sema;

	nat state;

	SemaInterop() : sema(0), state(0) {}

	void run() {
		state = 1;

		// Should block.
		sema.down();

		state = 2;
	}

	void small() {
		state = 5;
	}

	void run2() {
		state = 3;
		sema.down();
		state = 4;
	}
};

BEGIN_TEST(UThreadSemaInterop, OS) {
	SemaInterop t;
	ThreadGroup group;

	// Make the main thread wait for a UThread that is not in its running state.
	{
		os::Thread on = os::Thread::spawn(util::memberVoidFn(&t, &SemaInterop::run), group);
		Sleep(30);
		CHECK_EQ(t.state, 1);

		// Another thread, should block.
		UThread::spawn(util::memberVoidFn(&t, &SemaInterop::run2), &on);
		Sleep(30);
		CHECK_EQ(t.state, 3);
	}

	// Now, release the original thread and make sure it does not exit until the 'run2' call is complete.
	t.sema.up();
	Sleep(30);
	CHECK_EQ(t.state, 2);

	t.sema.up();
	Sleep(30);
	CHECK_EQ(t.state, 4);

	// No threads to schedule when a sema should block. This should make
	// the other thread spin in UThread::wait() for a while.
	os::Thread::spawn(util::memberVoidFn(&t, &SemaInterop::run), group);
	Sleep(30);
	CHECK_EQ(t.state, 1);
	t.sema.up();
	Sleep(30);
	CHECK_EQ(t.state, 2);

	// No threads to schedule when a sema would block while starting another UThread.
	{
		os::Thread on = os::Thread::spawn(util::memberVoidFn(&t, &SemaInterop::run), group);
		Sleep(30);
		CHECK_EQ(t.state, 1);

		// Launch another UThread!
		UThread::spawn(util::memberVoidFn(&t, &SemaInterop::small), &on);
		Sleep(30);
		CHECK_EQ(t.state, 5);

		t.sema.up();
	}
	Sleep(30);
	CHECK_EQ(t.state, 2);

} END_TEST
