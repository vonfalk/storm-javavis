#include "stdafx.h"

#include "Thread.h"
#include <process.h>

static const uintptr_t invalidId = -1L;

Thread::Thread() : threadId(invalidId), end(false), startSema(0), stopSema(0), running(false) {}

Thread::~Thread() {
	stopWait();
}

bool Thread::isRunning() {
	bool r = threadId != invalidId;
	if (running && !r) {
		// It stopped! Fix the semaphore.
		stopSema.down();
		running = false;
	}
	return r;
}

void Thread::stop() {
	end = true;
}

void Thread::stopWait() {
	stop();
	if (running) {
		// Optimization:
		stopSema.down();
		running = false;
	}
}

bool Thread::sameAsCurrent() {
	return GetCurrentThreadId() == threadId;
}

bool Thread::start(const Fn<void, Thread::Control &> &fn) {
	if (isRunning())
		return false;
	end = false;

	this->fn = fn;
	running = true;

	_beginthread(threadProc, 0, this);
	startSema.down();

	return true;
}

void Thread::threadProc(void *param) {
	Thread *me = (Thread *)param;
	me->threadId = GetCurrentThreadId();
	// Signal init done.
	me->startSema.up();

	Thread::Control control(*me);
	me->fn(control);

	// done, clean up
	me->threadId = invalidId;
	me->stopSema.up();

	_endthread();
}

