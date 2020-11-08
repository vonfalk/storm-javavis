#pragma once
#include "WindowGraphics.h"

namespace gui {

	/**
	 * A generic surface associated with some window on the current system, to which it is possible
	 * to draw.
	 */
	class Surface : NoCopy {
	public:
		// Create.
		Surface(Size size, Float scale) : size(size), scale(scale) {}

		// Size of the drawing area.
		Size size;

		// Scale (to respect DPI).
		Float scale;

		// Create a Graphics object for this surface.
		virtual WindowGraphics *createGraphics(Engine &e) = 0;

		// Resize this surface.
		virtual void resize(Size size, Float scale) = 0;

		// Present this surface to the screen (usually a swap, but might involve other mechanisms).
		virtual bool present(bool waitForVSync) = 0;
	};

}
