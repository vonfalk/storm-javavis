#pragma once
#include "Container.h"
#include "Core/Event.h"

namespace gui {

	/**
	 * A frame is a window with a border, present on the desktop. Creating the Frame does not make it visible.
	 *
	 * The frame has a little special life time management. The frame will keep itself alive until it is closed.
	 */
	class Frame : public Container {
		STORM_CLASS;
	public:
		// Note: does not create an actual frame. Use 'create' below to do that.
		STORM_CTOR Frame(Str *title);
		STORM_CTOR Frame(Str *title, Size size);

		// Create the frame and show it.
		void STORM_FN create();

		// Close this frame.
		virtual void STORM_FN close();

		// Wait until this frame is closed.
		void STORM_FN waitForClose();

#ifdef GUI_WIN32
		// Message!
		virtual MsgResult onMessage(const Message &msg);
#endif

#ifdef GUI_GTK
		// Window text.
		using Window::text;
		virtual void STORM_SETTER text(Str *str);

		// Container.
		virtual GtkFixed *container();
#endif

		// Set size.
		virtual void STORM_FN size(Size s);

		// Set position.
		virtual void STORM_SETTER pos(Rect r);
		using Window::pos;

		// Set fullscreen mode.
		void STORM_SETTER fullscreen(Bool f);
		Bool STORM_FN fullscreen();

		// Hide cursor on this window.
		void STORM_SETTER cursorVisible(Bool v);
		Bool STORM_FN cursorVisible();

	private:
		// Helper to create the window.
		bool createWindow(bool sizeable);

		// Event that fires when we're closed.
		Event *onClose;

		// Info. Not valid if we're not in fullscreen mode.
		Long windowedStyle;
		Rect windowedRect;

		// Fullscreen mode?
		Bool full;

		// Cursor visible?
		Bool showCursor;
	};

}
