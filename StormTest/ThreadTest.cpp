#include "stdafx.h"
#include "Test/Test.h"

static int var = 0;

void otherThread(Thread::Control &c) {
	var++;
	assert(c.thread().sameAsCurrent());
	Sleep(10);
	var++;
}

BEGIN_TEST(ThreadTest) {

	var = 0;
	{
		Thread t;
		t.start(simpleFn(otherThread));
		t.stopWait();
		CHECK_EQ(var, 2);

		t.start(simpleFn(otherThread));
	}
	CHECK_EQ(var, 4);

} END_TEST
