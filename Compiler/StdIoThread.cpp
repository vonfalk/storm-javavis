#include "stdafx.h"
#include "StdIoThread.h"

namespace storm {

	StdIo::StdIo() : first(null), last(null) {
		thread = CreateThread(NULL, 1024 * 3, &ioMain, this, 0, NULL);
	}

	StdIo::~StdIo() {
		StdRequest quit(stdIn, null, 0);
		post(&quit);

		// Wait for the thread to terminate.
		WaitForSingleObject(thread, 500);
	}

	void StdIo::post(StdRequest *r) {
		{
			util::Lock::L z(lock);
			if (last == null) {
				// One and only node!
				last = first = r;
			} else {
				last->next = r;
				last = r;
			}
		}

		cond.signal();
	}


	static void doRead(HANDLE handle, StdRequest *r) {
		DWORD result = 0;
		ReadFile(handle, r->dest, r->count, &result, NULL);
		r->count = Nat(result);
		r->wait.up();
	}

	static void doWrite(HANDLE handle, StdRequest *r) {
		DWORD result = 0;
		WriteFile(handle, r->dest, r->count, &result, NULL);
		r->count = Nat(result);
		r->wait.up();
	}

	DWORD WINAPI ioMain(void *param) {
		StdIo *me = (StdIo *)param;
		HANDLE handles[3];
		handles[stdIn] = GetStdHandle(STD_INPUT_HANDLE);
		handles[stdOut] = GetStdHandle(STD_OUTPUT_HANDLE);
		handles[stdError] = GetStdHandle(STD_ERROR_HANDLE);

		TODO(L"Note: currently input is blocking output, which is bad. We should do something clever about that!");

		while (true) {
			StdRequest *now = null;
			{
				util::Lock::L z(me->lock);
				now = me->first;
				if (now)
					me->first = now->next;
				if (me->last == now)
					me->last = null;
			}

			// Anything to do?
			if (now == null) {
				me->cond.wait();
				continue;
			}

			// Got a request!
			if (now->dest == null) {
				// We shall terminate...
				return 0;
			}

			if (now->stream == stdIn) {
				// Special care is needed when reading...
				doRead(handles[stdIn], now);
			} else {
				doWrite(handles[stdOut], now);
			}
		}
	}
}
