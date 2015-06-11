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

		// TODO: Make this a 'virtual' field.
		virtual void STORM_FN show();
		virtual void STORM_FN show(Bool show);

		// Call when initialization is done. This will take care of showing the window, among other things.
		// Only needed for objects that you subclass yourself (eg. Frame, ...).
		virtual void STORM_FN initDone();

		// Update the window (ie repaint it).
		virtual void STORM_FN update();

	protected:
		// Create a window, and handle it. Makes sure that all messages are handled correctly.
		// Equivalent to handle(CreateWindowEx(...)), but ensures that any messages sent before
		// CreateWindowEx returns are sent to this class as well.
		bool create(LPCTSTR className, LPCTSTR windowName, DWORD style,
					int x, int y, int w, int h,
					HWND parent, HMENU menu);
		bool createEx(DWORD exStyle, LPCTSTR className, LPCTSTR windowName, DWORD style,
					int x, int y, int w, int h,
					HWND parent, HMENU menu);

	private:
		HWND myHandle;
		Frame *root;

		// Internal helper for paint events.
		void onPaint();
	};

}
