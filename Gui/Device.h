#pragma once
#include "Handle.h"

namespace gui {

	/**
	 * A generic interface to some rendering device.
	 *
	 * An instance of this class represents some policy of how to create a rendering context,
	 * possibly with some set of associated resources.
	 *
	 * This interface is made to support at least:
	 * - (GDI+)
	 * - Direct 2D (Windows)
	 * - Cairo (Both HW and SW)
	 * - Skia (HW, Linux)
	 */
	class Device : NoCopy {
	public:
		// Create.
		Device(Size size, Float scale) : size(size), scale(scale) {}

		// Size of the drawing area.
		Size size;

		// Scale (to respect DPI).
		Float scale;

		// Create a surface to draw on associated with a window.
		// Might return "null", which means that the window is not yet ready for being rendered to.
		virtual Surface *createSurface(Handle window) = 0;
	};

}
