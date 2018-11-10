#pragma once
#include "Utils/Bitmask.h"
#include "Message.h"
#include "Handle.h"

namespace gui {

	/**
	 * Mouse event declarations.
	 */

	namespace mouse {
		// Mouse buttons.
		enum MouseButton {
			left, middle, right
		};
	}

#ifdef GUI_WIN32
	Point mousePos(const Message &msg);
	Point mouseAbsPos(Handle window, const Message &msg);
#endif

}
