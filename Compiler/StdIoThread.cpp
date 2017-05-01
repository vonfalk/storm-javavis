#include "stdafx.h"
#include "StdIoThread.h"

namespace storm {

	StdIo::StdIo() : first(null), last(null) {
		thread = NULL;
		handles[stdIn] = GetStdHandle(STD_INPUT_HANDLE);
		handles[stdOut] = GetStdHandle(STD_OUTPUT_HANDLE);
		handles[stdError] = GetStdHandle(STD_ERROR_HANDLE);
		InitializeCriticalSection(&inputLock);
	}

	StdIo::~StdIo() {
		if (thread != NULL) {
			StdRequest quit(stdIn, null, 0);
			postThread(&quit);

			// Wait for the thread to terminate.
			WaitForSingleObject(thread, 500);
		}
		DeleteCriticalSection(&inputLock);
	}

	static void doRead(HANDLE handle, StdRequest *r) {
		DWORD result = 0;
		ReadFile(handle, r->buffer, r->count, &result, NULL);
		r->count = Nat(result);
		r->wait.up();
	}

	static bool tryRead(HANDLE handle, StdRequest *r) {
		// NOTE: This does not seem to work... At least not when inside of Emacs...
		// Stdin handle becomes signaling when there is data to read.
		if (WaitForSingleObject(handle, 0) == WAIT_OBJECT_0) {
			// Safe to read!
			doRead(handle, r);
			return true;
		} else {
			return false;
		}
	}

	static void doWrite(HANDLE handle, StdRequest *r) {
		DWORD result = 0;
		WriteFile(handle, r->buffer, r->count, &result, NULL);
		r->count = Nat(result);
		r->wait.up();
	}

	static void doNop(StdRequest *r) {
		r->count = 0;
		r->wait.up();
	}

	void StdIo::post(StdRequest *r) {
		// No accidental 'terminate' messages.
		if (!r->buffer) {
			doNop(r);
			return;
		}

		switch (r->stream) {
		case stdIn: {
			// See if we can read now, other post to the thread.
			bool ok = false;
			// Note: 'tryRead' is sadly not reliable...
			if (false && TryEnterCriticalSection(&inputLock)) {
				ok = tryRead(handles[r->stream], r);
				LeaveCriticalSection(&inputLock);
			}

			if (!ok)
				postThread(r);
			break;
		}
		case stdOut:
		case stdError:
			// Output to the streams directly.
			// TODO: We should queue these requests if we realize they will take time.
			doWrite(handles[r->stream], r);
			break;
		default:
			// Invalid stream...
			doNop(r);
			break;
		}
	}

	void StdIo::postThread(StdRequest *r) {
		{
			util::Lock::L z(lock);
			if (last == null) {
				// One and only node!
				last = first = r;
			} else {
				last->next = r;
				last = r;
			}

			// Is the thread actually started?
			if (thread == NULL)
				thread = CreateThread(NULL, 1024 * 3, &ioMain, this, 0, NULL);
		}

		cond.signal();
	}


	DWORD WINAPI ioMain(void *param) {
		StdIo *me = (StdIo *)param;

		// Note: currently input is blocking output, which is bad. We should do something clever about that!

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
			if (now->buffer == null) {
				// We shall terminate...
				return 0;
			}

			if (now->stream == stdIn) {
				// Special care is needed when reading...
				EnterCriticalSection(&me->inputLock);
				doRead(me->handles[stdIn], now);
				LeaveCriticalSection(&me->inputLock);
			} else {
				doWrite(me->handles[stdOut], now);
			}
		}
	}
}
