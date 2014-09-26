#pragma once

class Runnable {
public:
	Runnable();

	virtual UINT run() = 0;

protected:
	bool isRunning, endThread;
	CWinThread *currentThread;
	void startThread(UINT priority = THREAD_PRIORITY_NORMAL);
	void stopThread();
	void stopWait();

	friend UINT runnable_start(LPVOID data);
};