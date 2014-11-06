#pragma once

#include "Function.h"
#include "Platform.h"
#include "Semaphore.h"

class Thread : NoCopy {
public:
	Thread();
	~Thread();

	bool isRunning();

	class Control : NoCopy {
	public:
		inline Thread &thread() { return t; };
		inline bool run() { return !t.end; };

		friend class Thread;
	protected:
		Control(Thread &t) : t(t) {};

		Thread &t;
	};

	bool start(const Fn<void, Thread::Control &> &fn);

	void stop();
	void stopWait();

	bool sameAsCurrent();

private:
	// Running thread's thread id.
	uintptr_t threadId;

	// Exit request?
	bool end;

	// Last known state of the thread.
	bool running;

	// The function to start
	Fn<void, Thread::Control &> fn;

	// Wait semaphores.
	Semaphore startSema;
	Semaphore stopSema;

	static void threadProc(void *param);
};

