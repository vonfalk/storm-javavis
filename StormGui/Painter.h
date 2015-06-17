#pragma once

namespace stormgui {
	class Window;

	/**
	 * Painter interface for window graphics. Running in a separate thread for now.
	 * Attach it to a window, and it will receive rendering notifications.
	 */
	class Painter : public ObjectOn<Render> {
		STORM_CLASS;
	public:
		STORM_CTOR Painter();
		~Painter();

		// Called whenever the drawable area has been resized.
		void STORM_FN resized(Size size);

		// Called to render the window. Return 'true' if you want to render the next frame as well.
		// Before this call, the background is filled with the background color specified.
		Bool STORM_FN render();

		// Remember the last size.
		STORM_VAR Size size;

		/**
		 * The following API is intended to be private to the C++ code.
		 */

		// Attach to a specific window.
		void CODECALL attach(Par<Window> to);

		// Detach from the previous window.
		void CODECALL detach();

		// Called when the attached window has been resized.
		void CODECALL resize(Size to);

		// Called when the attached window wants to be repainted.
		void CODECALL repaint();

	private:
		// Attached HWND.
		HWND attachedTo;

		// Current render target.
		ID2D1HwndRenderTarget *target;

		// Create any resources connected to the current device.
		void create();

		// Destroy any resources connected to the current device.
		void destroy();
	};

}

