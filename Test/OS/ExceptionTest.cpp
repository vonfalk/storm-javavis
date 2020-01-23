#include "stdafx.h"
#include "OS/Thread.h"
#include "OS/ThreadGroup.h"
#include "OS/Future.h"

static void error() {
	throw UserError(L"ERROR");
}

class PtrError : public os::PtrThrowable {
public:
	virtual const wchar *toCStr() const { return S("ERROR"); }
};

static void errorPtr() {
	PtrError *e = new PtrError();
	throw e;
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

	void runPtr() {
		try {
			errorPtr();
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

BEGIN_TEST(ExceptionPtrTest, OS) {
	ExThread z;
	os::ThreadGroup g;

	os::Thread::spawn(util::memberVoidFn(&z, &ExThread::runPtr), g);

	try {
		z.result.result();
		CHECK(false);
	} catch (const PtrError *ex) {
		CHECK(true);
		delete ex;
	} catch (...) {
		CHECK(false);
	}

	g.join();

} END_TEST
