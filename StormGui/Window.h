#pragma once
#include "Message.h"

namespace stormgui {

	class Frame;

	/**
	 * Base class for windows and controls.
	 *
	 * The Window class itself does not create a window, this is done by a derived class. Most
	 * derived classes creates a window right away in their constructor.
	 *
	 * All windows have exactly one parent, except for frames which have none (technically their
	 * parent are the desktop, but we do not implement that here). Re-parenting of windows is not
	 * supported. The 'root' member of a Frame is set to the object itself.
	 */
	class Window : public ObjectOn<Ui> {
		STORM_CLASS;
	public:
		// Create with nothing.
		STORM_CTOR Window();

		// Destroy.
		~Window();

		// Invalid window handle.
		static const HWND invalid;

		// Get our handle. May be 'invalid'.
		HWND handle() const;

		// Set our handle, our parent is inferred from the handle itself.
		void handle(HWND handle);

		// Root frame.
		Frame *STORM_FN rootFrame();

		// Called when a regular message has been received. If no result is returned, calls the default window proc,
		// or any message procedure declared by the window we're handling.
		virtual MsgResult onMessage(const Message &msg);

	private:
		HWND myHandle;
		Frame *root;
	};

}
