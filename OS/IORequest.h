#pragma once
#include "Sync.h"

namespace os {

	/**
	 * IO request. These are posted to an IOHandle when the requests are completed.
	 */
#ifdef WINDOWS

	class IORequest : public OVERLAPPED {
	public:
		IORequest();

		// Sema used for notifying when the request is complete.
		Sema wake;

		// Number of bytes read.
		nat bytes;

		// Called on completion.
		void complete(nat bytes);
	};

#else
#error "Implement IORequest for your platform!"
#endif

}
