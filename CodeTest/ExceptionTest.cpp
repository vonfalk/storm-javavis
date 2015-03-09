#include "stdafx.h"
#include "Test/Test.h"
#include "Code/Thread.h"
#include "Code/Future.h"

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

	Thread::spawn(memberVoidFn(&z, &ExThread::run));

	try {
		z.result.result();
		CHECK(false);
	} catch (const UserError &) {
		CHECK(true);
	} catch (...) {
		CHECK(false);
	}

	Sleep(100);

} END_TEST
