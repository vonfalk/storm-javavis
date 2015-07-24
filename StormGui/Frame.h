#pragma once
#include "Container.h"

namespace stormgui {

	// Position data from when we were not in fullscreen mode.
	struct FrameInfo {
		LONG style;
		RECT rect;
	};


	/**
	 * A frame is a window with a border, present on the desktop. Creating the Frame does not make it visible.
	 *
	 * The frame has a little special life time management. The frame will keep itself alive until it is closed.
	 */
	class Frame : public Container {
		STORM_CLASS;
	public:
		// Note: does not create an actual frame. Use 'create' below to do that.
		STORM_CTOR Frame(Par<Str> title);

		// Create the frame and show it.
		void STORM_FN create();

		// Close this frame.
		virtual void STORM_FN close();

		// Wait until this frame is closed.
		void STORM_FN waitForClose();

		// Message!
		virtual MsgResult onMessage(const Message &msg);

		// Set size.
		virtual void STORM_FN size(Size s);

		// Set position.
		virtual void STORM_SETTER pos(Rect r);
		using Window::pos;

		// Set fullscreen mode.
		void STORM_SETTER fullscreen(Bool f);
		Bool STORM_FN fullscreen();

	private:
		// Helper to create the window.
		bool createWindow(bool sizeable);

		// Event that fires when we're closed.
		os::Event onClose;

		// Fullscreen mode?
		bool full;

		// Info. Not valid if we're not in fullscreen mode.
		FrameInfo info;
	};

}
