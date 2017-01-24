#pragma once
#include "Handle.h"

namespace os {

	class ThreadData;

	/**
	 * IO handle. Encapsulates an OS specific handle to some kind of synchronizing object that the
	 * OS will notify when IO requests have been completed.
	 */
#ifdef WINDOWS

	class IOHandle {
	public:
		IOHandle() : handle(NULL) {}
		IOHandle(HANDLE h) : handle(h) {}
		inline operator bool () const { return handle != NULL; }
		inline HANDLE v() const { return handle; }

		// Associate a handle to this IO handle.
		void add(Handle h, const ThreadData *id);

		// Process all messages for this IO handle.
		void notifyAll(const ThreadData *id) const;

		// Close this handle.
		void close();

	private:
		HANDLE handle;
	};

#else
#error "Implement IOHandle for your platform!"
#endif

}
