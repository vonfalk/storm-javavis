#pragma once

namespace gui {

	/**
	 * Type of device. Determines how the Painter interacts with the render manager.
	 */
	enum DeviceType {
		// A device that is able to update the screen at any time, without asking the UI framework.
		// For example: Direct2D (call Present()), and raw OpenGL (call SwapBuffers, at least on X11).
		dtRaw,

		// A device that needs to interact with the window system to present the rendered contents,
		// which eventually means blitting the pixels to another surface somewhere.
		// This means that the Painter will invalidate the window in need of being updated, and rely
		// on the windowing system to keep track of repaints rather than the RenderMgr.
		// For example: Cairo (software and OpenGL), raw OpenGL under Wayland (creating sub-windows is not an option there).
		dtBlit,
	};

}

#include "DxDevice.h"
#include "CairoDevice.h"
