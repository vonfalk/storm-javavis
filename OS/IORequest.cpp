#include "stdafx.h"
#include "IORequest.h"
#include "Thread.h"

namespace os {

#ifdef WINDOWS

	IORequest::IORequest(const Thread &thread) : wake(0), bytes(0), error(0), thread(thread) {
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
		this->error = 0;
		wake.up();
	}

	void IORequest::failed(nat bytes, int error) {
		this->bytes = bytes;
		this->error = error;
		wake.up();
	}

#endif

#ifdef POSIX

	IORequest::IORequest(Handle handle, Type type, const Thread &thread) : type(type), handle(handle), thread(thread) {
		thread.threadData()->ioComplete.attach(handle, this);
	}

	IORequest::~IORequest() {
		thread.threadData()->ioComplete.detach(handle);
	}

#endif

}
