#include "stdafx.h"
#include "Test/Test.h"
#include "Code/Thread.h"
#include "Code/Future.h"

#include "Utils/ExceptionFwd.h"

static void error() {
	throw UserError(L"ERROR");
}

struct ExThread {

	Future result;

	ExThread() : result(null, 0) {}

	void run() {
		try {
			error();
		} catch (...) {
			result.error();
		}
	}

};

// Test forwarding exceptions between threads.
BEGIN_TEST_(ExceptionTest) {
	ExThread z;

	Thread::spawn(memberVoidFn(&z, &ExThread::run));

	try {
		z.result.wait();
		CHECK(false);
	} catch (const UserError &e) {
		PVAR(e);
		CHECK(true);
	} catch (...) {
		CHECK(false);
	}

	Sleep(2000);

} END_TEST
