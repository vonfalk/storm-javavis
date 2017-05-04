#pragma once
#include "OS/Handle.h"

namespace os {

	class ThreadData;

	/**
	 * IO handle. Encapsulates an OS specific handle to some kind of synchronizing object that the
	 * OS will notify when IO requests have been completed.
	 */
#ifdef WINDOWS

	class IOHandle {
	public:
		IOHandle();
		IOHandle(HANDLE h);

		// Get the underlying handle (if any).
		HANDLE v() const;

		// Associate a handle to this IO handle.
		void add(Handle h, const ThreadData *id);

		// Process all messages for this IO handle.
		void notifyAll(const ThreadData *id) const;

		// Close this handle.
		void close();

		// Attach/detach IO requests.
		void attach();
		void detach();

	private:
		HANDLE handle;

		// # of requests pending.
		mutable size_t pending;
	};

#else
#error "Implement IOHandle for your platform!"
#endif

}
