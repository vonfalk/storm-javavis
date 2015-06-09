#include "stdafx.h"
#include "Frame.h"

namespace stormgui {

	Frame::Frame() {
		// Add an extra reference to keep ourselves alive!
		addRef();
	}

	void Frame::close() {
		if (handle() == invalid) {
			handle(invalid);
			release();
		}
	}

}
