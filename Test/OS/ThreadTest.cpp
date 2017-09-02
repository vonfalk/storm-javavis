#include "stdafx.h"
#include "OS/Thread.h"
#include "OS/ThreadGroup.h"
#include "Tracker.h"

using namespace os;

static int var = 0;
static THREAD int local = 0;
static Semaphore stopSema(0);

static void otherThread() {
	var++;
	local++;
	Sleep(10);
	var++;
	local++;
	assert(local == 2);
	stopSema.up();
}

BEGIN_TEST(ThreadTest, OS) {
	ThreadGroup g;

	var = 0;
	local = 1;
	{
		os::Thread t = os::Thread::spawn(util::simpleVoidFn(otherThread), g);
		CHECK_NEQ(t, os::Thread::current());
	}
	Sleep(30);
	stopSema.down();
	CHECK_EQ(var, 2);

	{
		os::Thread t = os::Thread::spawn(util::simpleVoidFn(otherThread), g);
		CHECK_NEQ(t, os::Thread::current());
	}
	stopSema.down();
	CHECK_EQ(var, 4);
	CHECK_EQ(local, 1);

} END_TEST

static void setVar() {
	var = 1;
	stopSema.up();
}

static void uthreadInterop() {
	UThread::spawn(util::simpleVoidFn(&setVar));
	// Note: we're not running leave here!
}

static void setVar2() {
	// Keep a reference, to confuse the runtime a bit.
	os::Thread t = os::Thread::current();
	UThread::leave();
	var = 2;
	stopSema.up();
}

static void uthreadInterop2() {
	UThread::spawn(util::simpleVoidFn(&setVar2));
	UThread::leave();
}

struct TInterop {
	os::Future<int, Semaphore> result;

	void run() {
		result.post(42);
	}
};

BEGIN_TEST(UThreadInterop, OS) {
	ThreadGroup g;

	var = 0;
	os::Thread::spawn(util::simpleVoidFn(uthreadInterop), g);
	stopSema.down();
	CHECK_EQ(var, 1);

	var = 0;
	os::Thread::spawn(util::simpleVoidFn(uthreadInterop2), g);
	stopSema.down();
	CHECK_EQ(var, 2);

	var = 0;
	os::Thread t = os::Thread::spawn(util::Fn<void, void>(), g);
	Sleep(20); // Make sure 't' enters the condition waiting.
	UThread::spawn(util::simpleVoidFn(setVar), &t);
	stopSema.down();
	CHECK_EQ(var, 1);

	// Check so that futures work with regular semaphores as well.
	{
		TInterop i;
		os::Thread::spawn(util::memberVoidFn(&i, &TInterop::run), g);
		CHECK_EQ(i.result.result(), 42);
		// Multiple times should also work.
		CHECK_EQ(i.result.result(), 42);
	}

} END_TEST

struct Cond {
	// Condition variable to use for testing.
	Condition c;

	// Semaphore to start the other thread.
	Semaphore start;

	// Semaphore to indicate an iteration is done.
	Semaphore done;

	// Shared variable to use for results.
	nat count;

	Cond() : start(0), done(0), count(0) {}

	// Run the other thread.
	void run() {
		for (nat i = 0; i < 10; i++) {
			start.down();
			c.wait();
			count++;
			done.up();
		}

	}
};

BEGIN_TEST(ConditionTest, OS) {
	Cond z;
	ThreadGroup g;

	os::Thread::spawn(util::memberVoidFn(&z, &Cond::run), g);

	z.start.up();
	Sleep(10); // Let the other thread run into c.wait()
	z.c.signal();
	z.done.down();
	CHECK_EQ(z.count, 1);

	z.c.signal();
	z.start.up();
	z.done.down();
	CHECK_EQ(z.count, 2);

	z.c.signal();
	z.c.signal();
	z.start.up();
	z.done.down();
	Sleep(10); // If it runs multiple times, make sure it is complete.
	CHECK_EQ(z.count, 3);

	// Stress test the rest of the times...
	for (nat i = 4; i <= 10; i++) {
		z.start.up();
		z.c.signal();
		z.done.down();
		CHECK_EQ(z.count, i);
	}

	// Now z.count is 10, and the other thread will terminate eventually.
} END_TEST;

