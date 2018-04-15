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

	void IOHandle::remove(Handle h, const ThreadData *id) {
		// Not needed.
	}

	void IOHandle::notifyAll(const ThreadData *id) const {
		if (!handle)
			return;

		while (true) {
			DWORD bytes = 0;
			ULONG_PTR key = 0;
			OVERLAPPED *request = NULL;
			BOOL ok = GetQueuedCompletionStatus(handle, &bytes, &key, &request, 0);
			int error = GetLastError();

			if (request) {
				// PLN(L"Got status: " << bytes << L", " << key << L", " << request << L", " << ok);
				if ((ULONG_PTR)id == key) {
					IORequest *r = (IORequest *)request;

					if (ok)
						r->complete(nat(bytes));
					else
						r->failed(nat(bytes), error);
				}
			} else {
				// Nothing was dequeued, abort.
				break;
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

	IOHandle::~IOHandle() {}

	static short type(IORequest::Type type) {
		switch (type) {
		case IORequest::read:
			return POLLIN;
		case IORequest::write:
			return POLLOUT;
		default:
			return 0;
		}
	}

	void IOHandle::attach(Handle h, IORequest *wait) {
		util::Lock::L z(lock);
		handles.put(h.v(), type(wait->type), wait);
	}

	void IOHandle::detach(Handle h, IORequest *wait) {
		util::Lock::L z(lock);
		for (nat pos = handles.find(h.v()); pos < handles.capacity(); pos = handles.next(pos)) {
			if (handles.valueAt(pos) == wait) {
				handles.remove(pos);
				break;
			}
		}
	}

	void IOHandle::notifyAll(const ThreadData *id) const {
		UNUSED(id);
		util::Lock::L z(lock);

		struct pollfd *wait = handles.data();

		// See if we need to ask the OS for new events...
		bool any = false;
		for (size_t i = 0; i < handles.capacity(); i++)
			if (wait[i + 1].fd >= 0 && wait[i + 1].revents)
				any = true;

		if (!any) {
			// Find new events.
			int r = poll(wait + 1, handles.capacity(), 0);

			// Any use checking for events?
			if (r <= 0)
				return;
		}

		for (size_t i = 0; i < handles.capacity(); i++) {
			if (wait[i + 1].revents) {
				// Something happened!
				if (IORequest *r = handles.valueAt(i))
					r->wake.set();
			}
			wait[i + 1].revents = 0;
		}
	}

	IOHandle::Desc IOHandle::desc() {
		util::Lock::L z(lock);
		Desc d = { handles.data(), handles.capacity() + 1 };
		return d;
	}

	void IOHandle::add(Handle h, const ThreadData *id) {
		// Nothing to do on POSIX.
	}

	void IOHandle::remove(Handle h, const ThreadData *id) {
		util::Lock::L z(lock);

		// Mark all pending things as 'complete' and remove them.
		for (nat pos = handles.find(h.v()); pos < handles.capacity(); pos = handles.find(h.v())) {
			IORequest *r = handles.valueAt(pos);
			r->closed = true;
			r->wake.set();

			// Remove!
			handles.remove(pos);
		}
	}

	void IOHandle::close() {
		// Nothing to do.
	}

#endif

}
