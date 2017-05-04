#include "stdafx.h"
#include "IORequest.h"
#include "Thread.h"

namespace os {

	IORequest::IORequest(Thread &thread) : wake(0), bytes(0), thread(thread) {
		Internal = 0;
		InternalHigh = 0;
		Offset = 0;
		OffsetHigh = 0;
		Pointer = NULL;
		hEvent = NULL;
		thread.threadData()->ioComplete.attach();
	}

	IORequest::~IORequest() {
		thread.threadData()->ioComplete.detach();
	}

	void IORequest::complete(nat bytes) {
		this->bytes = bytes;
		wake.up();
	}


}
