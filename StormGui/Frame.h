#pragma once
#include "Container.h"

namespace stormgui {

	/**
	 * A frame is a window with a border, present on the desktop. Creating the Frame does not make it visible.
	 *
	 * The frame has a little special life time management. The frame will keep itself alive until it is closed.
	 */
	class Frame : public Container {
		STORM_CLASS;
	public:
		// Note: does not create an actual frame. Use 'create' below to do that.
		STORM_CTOR Frame();

		// Create a frame with a default size (either system default, or remembered from earlier).
		void STORM_FN create(Par<Str> title);

		// Create a frame with a specific size.
		void STORM_FN create(Par<Str> title, Size size);

		// Close this frame.
		void STORM_FN close();

		// Wait until this frame is closed.
		void STORM_FN waitForClose();

		// Message!
		virtual MsgResult onMessage(const Message &msg);

	private:
		// Helper to create the window.
		bool createWindow(const String &title, Size size, bool sizeable);

		// Event that fires when we're closed.
		os::Event onClose;
	};

}
