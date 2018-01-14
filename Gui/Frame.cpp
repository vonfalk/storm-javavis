#include "stdafx.h"
#include "Frame.h"
#include "App.h"
#include "GtkSignal.h"

namespace gui {

	static void goFull(Handle wnd, Long &oldStyle, Rect &oldPos);
	static void goBack(Handle wnd, Long &oldStyle, Rect &oldPos);


	Frame::Frame(Str *title) : full(false), showCursor(true) {
		onClose = new (this) Event();
		attachParent(this);
		text(title);
		pos(Rect(-10000, -10000, -10002, -10002));
	}

	Frame::Frame(Str *title, Size size) : full(false), showCursor(true) {
		onClose = new (this) Event();
		attachParent(this);
		text(title);
		pos(Rect(Point(-10000, -10000), size));
	}

	void Frame::create() {
		if (handle() != invalid)
			return;

		createWindow(true);
	}

	void Frame::close() {
		if (handle() != invalid) {
			handle(invalid);
		}

		onClose->set();
	}

	void Frame::waitForClose() {
		App *app = gui::app(engine());
		app->waitForEvent(this, onClose);
	}

	void Frame::size(Size s) {
		if (!full) {
			Rect p = pos();
			p.size(s);
			pos(p);
		}
	}

	void Frame::pos(Rect r) {
		if (!full) {
			Window::pos(r);
		}
	}

	Bool Frame::cursorVisible() {
		return showCursor;
	}

	void Frame::fullscreen(Bool v) {
		if (created() && full != v) {
			if (v) {
				goFull(handle(), windowedStyle, windowedRect);
			} else {
				goBack(handle(), windowedStyle, windowedRect);
			}
		}

		full = v;
	}

	Bool Frame::fullscreen() {
		return full;
	}

#ifdef GUI_WIN32

	// Convert a client size to the actual window size.
	static Size clientToWindow(Size s, DWORD windowStyles, DWORD windowExStyles) {
		RECT r = { 0, 0, (LONG)s.w, (LONG)s.h };
		AdjustWindowRectEx(&r, windowStyles & ~WS_OVERLAPPED, FALSE, windowExStyles);
		return Size(Float(r.right - r.left), Float(r.bottom - r.top));
	}

	// Make 'wnd' go fullscreen.
	static void goFull(Handle wnd, Long &oldStyle, Rect &oldPos) {
		oldStyle = GetWindowLong(wnd.hwnd(), GWL_STYLE);
		RECT oldRect;
		GetWindowRect(wnd.hwnd(), &oldRect);
		oldPos = convert(oldRect);

		HMONITOR m = MonitorFromWindow(wnd.hwnd(), MONITOR_DEFAULTTONEAREST);
		MONITORINFO mi = { sizeof(mi) };
		GetMonitorInfo(m, &mi);

		SetWindowLong(wnd.hwnd(), GWL_STYLE, oldStyle & ~(WS_CAPTION | WS_THICKFRAME));

		LONG l = mi.rcMonitor.left;
		LONG r = mi.rcMonitor.right;
		LONG t = mi.rcMonitor.top;
		LONG b = mi.rcMonitor.bottom;
		SetWindowPos(wnd.hwnd(), NULL, l, t, r - l, b - t, SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
	}

	static void goBack(Handle wnd, Long &style, Rect &rect) {
		LONG l = LONG(rect.p0.x);
		LONG r = LONG(rect.p1.x);
		LONG t = LONG(rect.p0.y);
		LONG b = LONG(rect.p1.y);

		SetWindowLong(wnd.hwnd(), GWL_STYLE, LONG(style));
		SetWindowPos(wnd.hwnd(), NULL, l, t, r - l, b - t, SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
	}

	// Helper to create the actual handle.
	bool Frame::createWindow(bool sizeable) {
		DWORD exStyles = WS_EX_CONTROLPARENT;
		DWORD styles = 0;
		if (sizeable)
			styles = WS_OVERLAPPEDWINDOW;
		else
			styles = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;

		styles |= WS_CLIPCHILDREN;

		if (!createEx(NULL, styles, exStyles, NULL, 0, cManualVisibility | cAutoPos))
			return false;

		// Create child windows (little of a hack, sorry!)
		parentCreated(0);

		if (visible()) {
			ShowWindow(handle().hwnd(), TRUE);
			update();
		}

		if (full) {
			goFull(handle(), windowedStyle, windowedRect);
		}

		return true;
	}

	MsgResult Frame::onMessage(const Message &msg) {
		switch (msg.msg) {
		case WM_CLOSE:
			close();
			return msgResult(0);
		case WM_SETCURSOR:
			if (LOWORD(msg.lParam) == HTCLIENT && !showCursor) {
				SetCursor(NULL);
				return msgResult(TRUE);
			}
			break;
		}

		return Container::onMessage(msg);
	}

	void Frame::cursorVisible(Bool v) {
		// Will be altered soon anyway, we do not need to do anything here.
		showCursor = v;
	}

#endif
#ifdef GUI_GTK

	static void goFull(Handle wnd, Long &, Rect &) {
		GdkWindow *window = gtk_widget_get_window(wnd.widget());
		gdk_window_set_fullscreen_mode(window, GDK_FULLSCREEN_ON_CURRENT_MONITOR);
		gdk_window_fullscreen(window);
	}

	static void goBack(Handle wnd, Long &, Rect &) {
		GdkWindow *window = gtk_widget_get_window(wnd.widget());
		gdk_window_unfullscreen(window);
	}

	bool Frame::createWindow(bool sizeable) {
		GtkWidget *frame = gtk_window_new(GTK_WINDOW_TOPLEVEL);
		gtk_window_set_title((GtkWindow *)frame, text()->utf8_str());
		gtk_window_set_resizable((GtkWindow *)frame, sizeable);

		GtkWidget *container = basic_new();
		gtk_container_add(GTK_CONTAINER(frame), container);
		gtk_widget_show(container);

		Signal<void, Frame>::Connect<&Frame::close>::to(frame, "destroy", engine());

		Size sz = pos().size();
		if (sz.w > 0 && sz.h > 0)
			gtk_window_set_default_size((GtkWindow *)frame, sz.w, sz.h);

		handle(frame);
		initSignals(frame);

		// Create child windows (if any).
		parentCreated(0);

		// Set visibility and full screen attributes.
		if (visible()) {
			gtk_widget_show(frame);
		}

		// Update cursor visibility.
		// cursorVisible(showCursor);
		cursorVisible(false);

		if (full) {
			goFull(handle(), windowedStyle, windowedRect);
		}

		return true;
	}

	void Frame::text(Str *str) {
		if (created()) {
			gtk_window_set_title((GtkWindow *)handle().widget(), str->utf8_str());
		}
		Window::text(str);
	}

	Basic *Frame::container() {
		// There is a BASIC layout inside the frame.
		return BASIC(gtk_bin_get_child(GTK_BIN(handle().widget())));
	}

	GtkWidget *Frame::drawWidget() {
		return gtk_bin_get_child(GTK_BIN(handle().widget()));
	}

	void Frame::cursorVisible(Bool v) {
		showCursor = v;
		if (created()) {
			GtkWidget *widget = handle().widget();
			GdkCursor *cursor = null;
			if (showCursor)
				cursor = gdk_cursor_new_from_name(gtk_widget_get_display(widget), "default");
			else
				cursor = gdk_cursor_new_for_display(gtk_widget_get_display(widget), GDK_BLANK_CURSOR);

			GdkWindow *window = gtk_widget_get_window(widget);

			gdk_window_set_cursor(window, cursor);
			g_object_unref((GObject *)cursor);
		}
	}

#endif

}
