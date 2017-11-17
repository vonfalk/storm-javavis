#pragma once
#include "Window.h"

namespace gui {

	/**
	 * A container is a window that can contain child windows.
	 *
	 * Manages IDs of child windows and forwards messages from child windows to their corresponding
	 * classes here.
	 */
	class Container : public Window {
		STORM_CLASS;
	public:
		// Create a container.
		STORM_CTOR Container();
	};

}
