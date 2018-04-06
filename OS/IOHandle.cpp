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

		while (true) {
			DWORD bytes = 0;
			ULONG_PTR key = 0;
			OVERLAPPED *request = NULL;
			BOOL ok = GetQueuedCompletionStatus(handle, &bytes, &key, &request, 0);

			if (request) {
				if ((ULONG_PTR)id == key) {
					IORequest *r = (IORequest *)request;

					if (ok)
						r->complete(nat(bytes));
					else
						r->failed(nat(bytes), GetLastError());
				}
				// PLN(L"Got status: " << bytes << L", " << key);
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

	IOHandle::IOHandle() : wait(null), capacity(0), waitValid(false) {}

	IOHandle::~IOHandle() {
		delete []wait;
	}

	void IOHandle::attach(Handle h, IORequest *wait) {
		util::Lock::L z(lock);
		handles[h.v()] = wait;
		waitValid = false;
	}

	void IOHandle::detach(Handle h) {
		util::Lock::L z(lock);
		HandleMap::iterator i = handles.find(h.v());
		if (i != handles.end()) {
			waitValid = false;
			handles.erase(i);
		}
	}

	void IOHandle::notifyAll(const ThreadData *id) const {
		UNUSED(id);
		util::Lock::L z(lock);

		for (size_t i = 1; i < capacity && wait[i].fd != 0; i++) {
			if (wait[i].revents != 0) {
				// Something happened!
				HandleMap::const_iterator found = handles.find(wait[i].fd);
				if (found != handles.end())
					found->second->wake.set();
			}
			wait[i].revents = 0;
		}
	}

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

	IOHandle::Desc IOHandle::desc() {
		util::Lock::L z(lock);

		if (!waitValid) {
			resize();
			size_t pos = 1;
			for (HandleMap::iterator i = handles.begin(), end = handles.end(); i != end; ++i) {
				wait[pos].fd = i->first;
				wait[pos].events = type(i->second->type);
				wait[pos].revents = 0;
				pos++;
			}
			for (; pos < capacity; pos++)
				wait[pos].fd = 0;
			waitValid = false;
		}

		Desc d = { wait, handles.size() + 1 };
		return d;
	}

	void IOHandle::resize() {
		size_t target = handles.size() + 1;
		size_t newCap = max(capacity, size_t(8));
		while (target > newCap)
			newCap *= 2;
		if (target < newCap / 4)
			newCap /= 4;
		if (newCap == capacity)
			return;

		// Resize! We do not need to preserve any contents.
		delete []wait;
		capacity = newCap;
		wait = new struct pollfd[capacity];
	}

	void IOHandle::add(Handle h, const ThreadData *id) {
		// Nothing to do on POSIX.
	}

	void IOHandle::close() {
		// Nothing to do.
	}

#endif

}
