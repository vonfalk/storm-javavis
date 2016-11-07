#include "stdafx.h"
#include "Compiler/Debug.h"
#include "Utils/Semaphore.h"

using namespace storm::debug;

static Link *createList(nat n) {
	Link *start = null;
	Link *prev = null;
	Engine &e = gEngine();

	for (nat i = 0; i < n; i++) {
		Link *now = new (e) Link;
		now->value = i;

		if (prev == null) {
			start = now;
		} else {
			prev->next = now;
		}
		prev = now;
	}

	// Trigger a GC to shake things up a bit!
	e.gc.collect();
	return start;
}

// Check a list.
static bool checkList(Link *first, nat n) {
	Link *at = first;
	for (nat i = 0; i < n; i++) {
		if (!at)
			return false;
		if (at->value != i)
			return false;

		at = at->next;
	}

	return at == null;
}

static Semaphore waitSema(0);
static bool threadOk = false;

static void threadFn() {
	gEngine().gc.attachThread();

	const nat count = 10000;

	Link *start = createList(count);
	threadOk = checkList(start, count);

	gEngine().gc.detachThread(os::Thread::current());
	waitSema.up();
}

BEGIN_TEST(GcThreadTest, GcThreads) {
	// Test so that other threads are properly GC:d.
	Engine &e = gEngine();

	os::Thread::spawn(util::simpleVoidFn(&threadFn), e.threadGroup);
	waitSema.down();
	CHECK(threadOk);

	// Now run in this thread as well to ensure we did not break anything by deattaching the thread.
	const nat count = 10000;
	Link *start = createList(count);
	CHECK(checkList(start, count));

} END_TEST

// Should be large enough to trigger a garbage collection.
static const nat count = 5000;

static void uthreadFn() {
	Link *start = createList(count);
	os::UThread::leave();
	threadOk = checkList(start, count);
}

BEGIN_TEST(GcUThreadTest, GcThreads) {
	Engine &e = gEngine();

	threadOk = false;

	os::UThread::spawn(util::simpleVoidFn(&uthreadFn));
	Link *start = createList(count);
	os::UThread::leave();
	createList(count);
	os::UThread::leave();

	// Should be done by now!
	CHECK(threadOk);
	CHECK(checkList(start, count));
} END_TEST
