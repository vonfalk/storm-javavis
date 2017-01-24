#include "stdafx.h"
#include "IOHandle.h"
#include "IORequest.h"

namespace os {

#ifdef WINDOWS

	void IOHandle::add(Handle h, const ThreadData *id) {
		HANDLE r = CreateIoCompletionPort(h.v(), v(), (ULONG_PTR)id, 1);
		if (r == NULL) {
			// This fails if the handle does not have the OVERLAPPED flag set.
			// PLN(L"ERROR: " << GetLastError());
		} else {
			handle = r;
		}
	}

	void IOHandle::notifyAll(const ThreadData *id) const {
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
		CloseHandle(handle);
		handle = NULL;
	}

#endif

}
