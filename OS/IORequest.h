#pragma once
#include "Sync.h"
#include "Handle.h"

namespace os {

	class Thread;

	/**
	 * IO request. These are posted to an IOHandle when the requests are completed.
	 */
#if defined(WINDOWS)

	class IORequest : public OVERLAPPED {
	public:
		// Note the thread that the request is associated with.
		IORequest(const Thread &thread);
		~IORequest();

		// Sema used for notifying when the request is complete.
		Sema wake;

		// Number of bytes read.
		nat bytes;

		// Error code (if any).
		int error;

		// Called on completion.
		void complete(nat bytes);

		// Called on failure.
		void failed(nat bytes, int error);

	private:
		// Owning thread.
		const Thread &thread;
	};

#elif defined(POSIX)

	class IORequest {
	public:
		// Read/write?
		enum Type {
			read, write
		};

		// Note the thread that the request is associated with.
		IORequest(Handle handle, Type type, const Thread &thread);
		~IORequest();

		// Event used for notifying when the file descriptor is readable.
		Event wake;

		// Request type (read/write).
		Type type;

	private:
		// Handle used.
		Handle handle;

		// Owning thread.
		const Thread &thread;
	};

#else
#error "Implement IORequest for your platform!"
#endif

}
