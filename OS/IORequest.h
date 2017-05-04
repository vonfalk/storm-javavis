#pragma once
#include "Sync.h"

namespace os {

	class Thread;

	/**
	 * IO request. These are posted to an IOHandle when the requests are completed.
	 */
#ifdef WINDOWS

	class IORequest : public OVERLAPPED {
	public:
		// Note the thread that the request is associated with.
		IORequest(Thread &thread);
		~IORequest();

		// Sema used for notifying when the request is complete.
		Sema wake;

		// Number of bytes read.
		nat bytes;

		// Called on completion.
		void complete(nat bytes);

	private:
		// Owning thread.
		Thread &thread;
	};

#else
#error "Implement IORequest for your platform!"
#endif

}
