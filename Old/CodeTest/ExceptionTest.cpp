#include "stdafx.h"
#include "Test/Lib/Test.h"
#include "OS/Thread.h"
#include "OS/ThreadGroup.h"
#include "OS/Future.h"

static void error() {
	throw UserError(L"ERROR");
}

struct ExThread {

	FutureSema<Semaphore> result;

	ExThread() : result(null) {}

	void run() {
		try {
			error();
		} catch (...) {
			result.error();
		}
	}

};

// Test forwarding exceptions between threads.
BEGIN_TEST(ExceptionTest) {
	ExThread z;
	ThreadGroup g;

	Thread::spawn(memberVoidFn(&z, &ExThread::run), g);

	try {
		z.result.result();
		CHECK(false);
	} catch (const UserError &) {
		CHECK(true);
	} catch (...) {
		CHECK(false);
	}

	g.join();

} END_TEST
