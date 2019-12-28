#include "stdafx.h"
#include "Compiler/Debug.h"
#include "Utils/Bitwise.h"
#include "Storm/Fn.h"

using namespace storm::debug;

BEGIN_TEST(GcTest1, GcScan) {
	Gc &g = gc();

	CHECK(g.test());
} END_TEST

BEGIN_TEST(GcTest2, GcObjects) {
	Engine &e = gEngine();

	// Allocate this many nodes to make sure MPS will try to GC at least once!
	const nat count = 100000;

	Link *start = createList(e, count);
	CHECK(checkList(start, count));

} END_TEST

/**
 * Long-running stresstest of the GC logic. Too slow for regular use, but good when debugging.
 */
BEGIN_TEST(GcTest3, Stress) {
	Engine &e = gEngine();

	// Allocate this many nodes.
	const nat count = 1000;

	for (nat z = 0; z < 10; z++) {
		ValClass *start = null;
		ValClass *prev = null;

		for (nat i = 0; i < count; i++) {
			ValClass *now = new (e) ValClass;
			now->data.value = i;
			now->data.list = createList(e, i + 1);

			if (prev) {
				prev->next = now;
			} else {
				start = now;
			}
			prev = now;
		}

		ValClass *at = start;
		for (nat i = 0; i < count; i++) {
			if (at->data.value != i) {
				CHECK_EQ(at->data.value, i);
				break;
			}
			if (!checkList(at->data.list, i + 1)) {
				CHECK(checkList(at->data.list, i + 1));
				break;
			}

			at = at->next;
		}

		CHECK_EQ(at, (ValClass *)null);
	}

} END_TEST

/**
 * Essentially the above test but using multiple threads.
 */

class GcThreadCtx {
public:
	bool ok;

	GcThreadCtx() : ok(false) {}

	void run() {
		Engine &e = gEngine();

		// Allocate this many nodes.
		const Nat count = 10000;

		// This many times.
		const Nat times = 4000;

		ok = true;
		for (Nat time = 0; time < times; time++) {
			Link *list = createList(e, count);
			if (!checkList(list, count))
				ok = false;
			os::UThread::leave();
		}
	}

};

BEGIN_TEST(GcThreadTest1, Stress) {
	Engine &e = gEngine();
	os::ThreadGroup group(util::memberVoidFn(&e, &Engine::attachThread),
						util::memberVoidFn(&e, &Engine::detachThread));

	GcThreadCtx a;
	GcThreadCtx b;
	GcThreadCtx c;

	os::Thread::spawn(util::memberVoidFn(&a, &GcThreadCtx::run), group);
	os::Thread::spawn(util::memberVoidFn(&b, &GcThreadCtx::run), group);
	c.run();

	group.join();
	CHECK(a.ok);
	CHECK(b.ok);
	CHECK(c.ok);
} END_TEST

BEGIN_TEST(GcThreadTest2, Stress) {
	Engine &e = gEngine();
	os::ThreadGroup group(util::memberVoidFn(&e, &Engine::attachThread),
						util::memberVoidFn(&e, &Engine::detachThread));

	GcThreadCtx a1;
	GcThreadCtx a2;
	GcThreadCtx b1;
	GcThreadCtx b2;
	GcThreadCtx c1;
	GcThreadCtx c2;

	{
		os::Thread t = os::Thread::spawn(util::memberVoidFn(&a1, &GcThreadCtx::run), group);
		os::UThread::spawn(util::memberVoidFn(&a2, &GcThreadCtx::run), &t);
		t = os::Thread::spawn(util::memberVoidFn(&b1, &GcThreadCtx::run), group);
		os::UThread::spawn(util::memberVoidFn(&b2, &GcThreadCtx::run), &t);
		os::UThread::spawn(util::memberVoidFn(&c2, &GcThreadCtx::run));
		c1.run();
	}

	group.join();
	CHECK(a1.ok);
	CHECK(a2.ok);
	CHECK(b1.ok);
	CHECK(b2.ok);
	CHECK(c1.ok);
	CHECK(c2.ok);
} END_TEST

BEGIN_TEST(GcThreadTest3, Stress) {
	CHECK(runFn<Bool>(S("tests.gc.testGcPost")));
} END_TEST

