#include "stdafx.h"
#include "IOHandle.h"
#include "IORequest.h"

namespace os {

#ifdef WINDOWS

	IOHandle::IOHandle() : handle(NULL), pending(0) {}

	IOHandle::IOHandle(HANDLE h) : handle(h), pending(0) {}

	HANDLE IOHandle::v() const {
		/**
		 * NOTE: Due to a bug? in windows 7, an IO Completion Port is signaled at all times when it
		 * is not associated with any file handles. Because of this, we keep track of the current
		 * number of pending IO requests and pretend that we do not have an IO Completion Port if
		 * there are no outstanding requests for this thread.
		 */
		if (atomicRead(pending) > 0)
			return handle;
		else
			return NULL;
	}

	void IOHandle::add(Handle h, const ThreadData *id) {
		HANDLE r = CreateIoCompletionPort(h.v(), handle, (ULONG_PTR)id, 1);
		if (r == NULL) {
			// This fails if the handle does not have the OVERLAPPED flag set.
			// PLN(L"ERROR: " << GetLastError());
		} else {
			handle = r;
		}
	}

	void IOHandle::notifyAll(const ThreadData *id) const {
		if (!handle)
			return;

		DWORD bytes = 0;
		ULONG_PTR key = 0;
		OVERLAPPED *request = NULL;
		while (GetQueuedCompletionStatus(handle, &bytes, &key, &request, 0)) {
			// PLN(L"Got status: " << bytes << L", " << key);
			if ((ULONG_PTR)id == key) {
				IORequest *r = (IORequest *)request;
				r->complete(nat(bytes));
			}
		}
	}

	void IOHandle::close() {
		if (handle)
			CloseHandle(handle);
		handle = NULL;
	}

	void IOHandle::attach() {
		atomicIncrement(pending);
	}

	void IOHandle::detach() {
		assert(atomicRead(pending) > 0);
		atomicDecrement(pending);
	}

#endif

#ifdef POSIX

	IOHandle::IOHandle() {}

	void IOHandle::add(Handle h, const ThreadData *id) {
		UNUSED(h);
		UNUSED(id);
	}

	void IOHandle::notifyAll(const ThreadData *id) const {
		UNUSED(id);
	}

	void IOHandle::close() {}

	void IOHandle::attach() {}

	void IOHandle::detach() {}


#endif

}
