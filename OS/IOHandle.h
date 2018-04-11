#pragma once
#include "OS/Handle.h"
#include "Utils/Lock.h"

#if defined(POSIX)
#include <poll.h>
#endif

namespace os {

	class ThreadData;
	class IORequest;

	/**
	 * IO handle. Encapsulates an OS specific handle to some kind of synchronizing object that the
	 * OS will notify when IO requests have been completed.
	 *
	 * TODO: We could use epoll on Linux for better performance when using many file descriptors.
	 */
#if defined(WINDOWS)

	class IOHandle {
	public:
		IOHandle();
		IOHandle(HANDLE h);

		// Get the underlying handle (if any).
		HANDLE v() const;

		// Associate a handle to this IO handle.
		void add(Handle h, const ThreadData *id);

		// Remove association with this IO handle.
		void remove(Handle h, const ThreadData *id);

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

#elif defined(POSIX)

	class IOHandle {
	public:
		IOHandle();
		~IOHandle();

		// Associate a handle to this IOHandle.
		void add(Handle h, const ThreadData *id);

		// Remove association of a handle.
		void remove(Handle h, const ThreadData *id);

		// Attach to this IO handle.
		void attach(Handle h, IORequest *request);

		// Detach from this IO handle.
		void detach(Handle h);

		// Process all messages for this IO handle.
		void notifyAll(const ThreadData *id) const;

		// Close this handle.
		void close();

		// Get an array of pollfd:s describing the threads waiting currently.
		struct Desc {
			struct pollfd *fds;
			size_t count;
		};
		Desc desc();

	private:
		// Lock, just in case.
		mutable util::Lock lock;

		// All handles currently associated with us.
		typedef map<int, IORequest *> HandleMap;
		HandleMap handles;

		// Previously allocated array of pollfd structs.
		struct pollfd *wait;

		// Number of entries in 'wait'. Any unused entries have their 'fd' set to zero.
		size_t capacity;

		// Is 'wait' properly updated?
		bool waitValid;

		// Resize 'wait' to an appropriate size.
		void resize();
	};

#else
#error "Implement IOHandle for your platform!"
#endif

}
