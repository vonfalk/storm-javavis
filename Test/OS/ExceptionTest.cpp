#include "stdafx.h"
#include "OS/Thread.h"
#include "OS/ThreadGroup.h"
#include "OS/Future.h"

static void error() {
	throw UserError(L"ERROR");
}

struct ExThread {

	os::FutureSema<Semaphore> result;

	ExThread() : result() {}

	void run() {
		try {
			error();
		} catch (...) {
			result.error();
		}
	}

};

BEGIN_TEST(ExceptionTest, OS) {
	ExThread z;
	os::ThreadGroup g;

	os::Thread::spawn(util::memberVoidFn(&z, &ExThread::run), g);

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
