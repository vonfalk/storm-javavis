#include "stdafx.h"

#include "Runnable.h"

Runnable::Runnable()  {
	isRunning = false;
	endThread = false;
	currentThread = null;
}

UINT runnable_start(LPVOID data) {
	if (data == 0) return -1;

	Runnable *r = (Runnable *)data;
	r->isRunning = true;
	r->endThread = false;
	UINT result = r->run();
	r->isRunning = false;
	r->currentThread = null;
	return result;
}

void Runnable::startThread(UINT priority) {
	if (this == 0)
		return;
	currentThread = AfxBeginThread(runnable_start, this, priority);
}

void Runnable::stopThread() {
	endThread = true;
}

void Runnable::stopWait() {
	stopThread();

	while (isRunning) {
		Sleep(1);
	}
}