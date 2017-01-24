#include "stdafx.h"
#include "IORequest.h"

namespace os {

	IORequest::IORequest() : wake(0), bytes(0) {
		Internal = 0;
		InternalHigh = 0;
		Offset = 0;
		OffsetHigh = 0;
		Pointer = NULL;
		hEvent = NULL;
	}

	void IORequest::complete(nat bytes) {
		this->bytes = bytes;
		wake.up();
	}


}
