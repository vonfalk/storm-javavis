#pragma once
#include "Message.h"
#include "Handle.h"
#include "Font.h"
#include "Key.h"
#include "Mouse.h"
#include "Core/Timing.h"
#include "Core/TObject.h"
#include "Utils/Bitmask.h"

namespace gui {
	class ContainerBase;
	class Frame;
	class Painter;
	class Timer;

	// Modifiers for the create function.
	enum CreateFlags {
		cNormal = 0x0,
		cManualVisibility = 0x1,
		cAutoPos = 0x2,
	};

	BITMASK_OPERATORS(CreateFlags);


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
		virtual ~Window();

		// Invalid window handle.
		static const Handle invalid;

		// Get our handle. May be 'invalid'.
		Handle handle() const;

		// Set our handle, our parent is inferred from the handle itself.
		void handle(Handle handle);

		// Are we created?
		bool created() const { return handle() != invalid; }

		// Parent/owner. Null for frames.
		MAYBE(ContainerBase *) STORM_FN parent();

		// Root frame. Returns ourself for frames, returns null before attached.
		MAYBE(Frame *) STORM_FN rootFrame();

		// Attach to a parent container and creation. To be called from 'container'.
		void attachParent(ContainerBase *parent);

		// Detach from our parent. This destroys the window.
		void detachParent();

		// Called when this window has been destroyed. This function is intended as a notification
		// for child classes in C++ (eg. Container). If you want to be notified when the window is
		// about to be destroyed, handle WM_CLOSE instead.
		virtual void windowDestroyed();

		// Note: 'parent' need to be set before calling this function. This initializes the creation of our window.
		virtual void parentCreated(nat id);

#ifdef GUI_WIN32
		// Called when a regular message has been received. If no result is returned, calls the
		// default window proc, or any message procedure declared by the window we're handling. Only
		// works for windows with the default window class provided by App. Otherwise, use 'preTranslateMessage'.
		virtual MsgResult onMessage(const Message &msg);

		// Called before a message is dispatched to the window procedure. Return a result to inhibit the regular
		// dispatch.
		virtual MsgResult beforeMessage(const Message &msg);

		// Called when a WM_COMMAND has been sent to (by) us. Return 'true' if it is handled. Type
		// is the notification code specified by the message (eg BN_CLICK).
		virtual bool onCommand(nat type);

		// Get the current DPI for this window. Only makes sense on Windows.
		virtual Nat currentDpi();

		// Update this window to new DPI settings (i.e., "currentDpi()" has changed).
		virtual void updateDpi(Bool move);
#endif
#ifdef GUI_GTK
		// Called when we received an EXPOSE event, but before Gtk+ is informed about the
		// event. Return 'true' to inhibit the Gtk+ behaviour completely.
		bool preExpose(GtkWidget *widget);

#endif

		// Visibility.
		Bool STORM_FN visible();
		void STORM_ASSIGN visible(Bool show);

		// Window text.
		virtual Str *STORM_FN text();
		virtual void STORM_ASSIGN text(Str *str);

		// Window position. Always relative to the client area (even in Frames).
		Rect STORM_FN pos();
		virtual void STORM_ASSIGN pos(Rect r);

		// Get the minimum size for this window. Note: This does not consider the size and position
		// of any child windows in case this is a container. This function is mostly useful for
		// determining the preferred size for various widgets.
		virtual Size STORM_FN minSize();

		// Font.
		Font *STORM_FN font();
		void STORM_ASSIGN font(Font *font);

		// Set focus.
		virtual void STORM_FN focus();

		// Update the window (ie repaint it) right now.
		virtual void STORM_FN update();

		// Repaint the window when we have time.
		virtual void STORM_FN repaint();

		// Called when the window is resized.
		virtual void STORM_FN resized(Size size);

		// Key events. Return 'true' if the message was handled and should not propagate further.
		virtual Bool STORM_FN onKey(Bool pressed, key::Key keycode, mod::Modifiers modifiers);
		virtual Bool STORM_FN onChar(Nat charCode);

		// Mouse events. Return 'true' if the message was handled and should not propagate further.
		virtual Bool STORM_FN onClick(Bool pressed, Point at, mouse::MouseButton button);
		virtual Bool STORM_FN onDblClick(Point at, mouse::MouseButton button);
		virtual Bool STORM_FN onMouseMove(Point at);
		virtual Bool STORM_FN onMouseVScroll(Point at, Int delta);
		virtual Bool STORM_FN onMouseHScroll(Point at, Int delta);

		// Mouse enter/mouse leave. Mouse enter always gets called, and is to return true if the
		// 'leave' notification is required. The mouse is considered inside the window only as long
		// as the mouse is directly on top of this window, i.e. not when the current window is
		// obscured by a child window.
		virtual void STORM_FN onMouseEnter();
		virtual void STORM_FN onMouseLeave();

		// Set window contents (custom drawing).
		void STORM_ASSIGN painter(MAYBE(Painter *) to);

		// Get the current painter.
		MAYBE(Painter *) STORM_FN painter();

		// Window timer.
		virtual void STORM_FN onTimer();

		// Set timer.
		void STORM_FN setTimer(Duration interval);

		// Stop timer.
		void STORM_FN clearTimer();

	protected:
		// Override this to do any special window creation. The default implementation creates a
		// plain child window with no window class. Called as soon as we know our parent (not on
		// Frames). Returns 'false' on failure.
		virtual bool create(ContainerBase *parent, nat id);

		// Internal 'resized' notification.
		virtual void onResize(Size size);

		// Destroy a handle.
		virtual void destroyWindow(Handle handle);

#ifdef GUI_WIN32
		// Create a window, and handle it. Makes sure that all messages are handled correctly.
		// Equivalent to handle(CreateWindowEx(...)), but ensures that any messages sent before
		// CreateWindowEx returns are sent to this class as well.
		// If NULL is passed as the class name, the default window class will be used.
		bool createEx(LPCTSTR className, DWORD style, DWORD exStyle, HWND parent, nat id, CreateFlags flags);
		bool createEx(LPCTSTR className, DWORD style, DWORD exStyle, HWND parent, nat id);
		bool createEx(LPCTSTR className, DWORD style, DWORD exStyle, HWND parent);
#endif
#ifdef GUI_GTK
		// Called to perform initialization of the recently created widget. Performs things such as
		// setting visibility, text and position. Also calls 'handle()' on the widget.
		void initWidget(ContainerBase *parent, GtkWidget *widget);

		// Initialize any signals required by the Window class.
		virtual void initSignals(GtkWidget *widget, GtkWidget *draw);

		// Get the widget we shall set the font on.
		virtual GtkWidget *fontWidget();

	public:
		// Get the widget we shall draw on.
		virtual GtkWidget *drawWidget();
#endif
	private:
		// Handle.
		Handle myHandle;

		// Parent.
		ContainerBase *myParent;

		// Root.
		Frame *myRoot;

		// Visible?
		Bool myVisible;

		// Currently drawing anything to this window?
		Bool drawing;

		// Is the mouse inside this window? (Used on Win32 to implement enter/leave notifications)
		Bool mouseInside;

		// Text.
		Str *myText;

	protected:
		// Position (Frame needs to modify this).
		Rect myPos;

	private:
		// Font.
		Font *myFont;

		// Painter.
		Painter *myPainter;

		// In Gtk+, widgets are usually not rendered in separate windows. When we're using OpenGL
		// rendering, we need separate windows for the widget being drawn to, and any child
		// widgets. This variable represents the created window.
		UNKNOWN(PTR_NOGC) GdkWindow *gdkWindow;

		// In Gtk+, we need to allocate a timer object separatly.
		UNKNOWN(PTR_NOGC) Timer *gTimer;

		// Timer timeout (nonzero = set).
		Duration timerInterval;

		// Prepare for a painter/prepare for no painter. Not called when we swap painter.
		void attachPainter();
		void detachPainter();

#ifdef GUI_WIN32
		// Handle on paint events.
		MsgResult onPaint();
#endif
#ifdef GUI_GTK
		// Signal landing pads.
		gboolean onKeyUp(GdkEvent *event);
		gboolean onKeyDown(GdkEvent *event);
		gboolean onButton(GdkEvent *event);
		gboolean onMotion(GdkEvent *event);
		gboolean onEnter(GdkEvent *event);
		gboolean onLeave(GdkEvent *event);
		gboolean onScroll(GdkEvent *event);
		void onSize(GdkRectangle *alloc);
		gboolean onDraw(cairo_t *ctx);
		void onRealize();
		void onUnrealize();

		// Set window mask.
		void setWindowMask(GdkWindow *window);

		// Do we need to be a separate native window?
		bool useNativeWindow();
#endif
	};

}
