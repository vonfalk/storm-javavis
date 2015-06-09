#pragma once
#include "Window.h"

namespace stormgui {

	/**
	 * A frame is a window with a border, present on the desktop. Creating the Frame does not make it visible.
	 *
	 * The frame has a little special life time management. The frame will keep itself alive until it is closed.
	 */
	class Frame : public Window {
		STORM_CLASS;
	public:
		STORM_CTOR Frame();

		// Close this frame.
		void STORM_FN close();

	};

}
