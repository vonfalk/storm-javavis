#include "stdafx.h"
#include "Test/Test.h"
#include "Compiler/Debug.h"
#include "Utils/Semaphore.h"

using namespace storm::debug;

static Link *createList(nat n) {
	Link *start = null;
	Link *prev = null;

	for (nat i = 0; i < n; i++) {
		Link *now = new (*gEngine) Link;
		now->value = i;

		if (prev == null) {
			start = now;
		} else {
			prev->next = now;
		}
		prev = now;
	}

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
	gEngine->gc.attachThread();

	const nat count = 100000;

	Link *start = createList(count);
	threadOk = checkList(start, count);

	gEngine->gc.detachThread(os::Thread::current());
	waitSema.up();
}

BEGIN_TEST(ThreadTest) {
	// Test so that other threads are properly GC:d. Note: we can not yet use UThreads.
	Engine &e = *gEngine;

	os::Thread::spawn(simpleVoidFn(&threadFn), e.threadGroup);
	waitSema.down();
	CHECK(threadOk);

	// Now run in this thread as well to ensure we did not break anything by deattaching the thread.
	const nat count = 100000;
	Link *start = createList(count);
	CHECK(checkList(start, count));

} END_TEST
