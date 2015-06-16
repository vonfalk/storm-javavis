#pragma once

namespace stormgui {

	/**
	 * Painter interface for window graphics. Running in a separate thread for now.
	 * Attach it to a window, and it will receive rendering notifications.
	 */
	class Painter : public ObjectOn<Render> {
		STORM_CLASS;
	public:
		STORM_CTOR Painter();

		// Called to render the window. Return 'true' if you want to render the next frame as well.
		// Before this call, the background is filled with the background color specified.
		Bool STORM_FN render(Size size);

	};

}

