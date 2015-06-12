#pragma once
#include "Message.h"
#include "Utils/Bitmask.h"

namespace stormgui {

	class Container;
	class Frame;

	/**
	 * Base class for windows and controls.
	 *
	 * The Window class itself creates a plain, empty window that can be used for drawing custom
	 * controls. The underlying window is not created directly, instead it is delayed until the
	 * window has been attached to a parent of some sort. The exception from this rule are Frames,
	 * which has no parents and therefore require special attention (see documentation for Frame).
	 *
	 * The fact that windows are not created until attached are hidden as good as possible. This
	 * means that we have to cache all properties as good as possible until the window is actually
	 * created.
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

		// Are we created?
		bool created() const { return handle() != invalid; }

		// Parent/owner. Null for frames.
		MAYBE(Container) *STORM_FN parent();

		// Root frame. Returns ourself for frames, returns null before attached.
		MAYBE(Frame) *STORM_FN rootFrame();

		// Attach to a parent container and creation. To be called from 'container'.
		void attachParent(Container *parent);

		// Note: 'parent' need to be set before calling this function. This initializes the creation of our window.
		virtual void parentCreated(nat id);

		// Called when a regular message has been received. If no result is returned, calls the default window proc,
		// or any message procedure declared by the window we're handling.
		virtual MsgResult onMessage(const Message &msg);

		// Visibility.
		Bool STORM_FN visible();
		void STORM_SETTER visible(Bool show);

		// Window text.
		Str *STORM_FN text();
		void STORM_SETTER text(Par<Str> str);
		void text(const String &str);

		// Window position. Always relative to the client area (even in Frames).
		Rect STORM_FN pos();
		void STORM_SETTER pos(Rect r);

		// Update the window (ie repaint it).
		virtual void STORM_FN update();

		// Called when the window is resized.
		virtual void STORM_FN resized(Size size);

		// Modifiers for the create function.
		enum CreateFlags {
			cNormal = 0x0,
			cManualVisibility = 0x1,
			cAutoPos = 0x2,
		};

	protected:
		// Override this to do any special window creation. The default implementation creates a
		// plain child window with no window class. Called as soon as we know our parent (not on
		// Frames). Returns 'false' on failure.
		virtual bool create(HWND parent, nat id);

		// Create a window, and handle it. Makes sure that all messages are handled correctly.
		// Equivalent to handle(CreateWindowEx(...)), but ensures that any messages sent before
		// CreateWindowEx returns are sent to this class as well.
		// If NULL is passed as the class name, the default window class will be used.
		bool createEx(LPCTSTR className, DWORD style, DWORD exStyle, HWND parent, nat id = 0, CreateFlags flags = cNormal);

	private:
		HWND myHandle;

		// Parent.
		Container *myParent;

		// Root.
		Frame *myRoot;

		// Visible?
		bool myVisible;

		// Text.
		String myText;

		// Position.
		Rect myPos;
	};

	BITMASK_OPERATORS(Window::CreateFlags);
}
