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

	void Frame::cursorVisible(Bool v) {
		// Will be altered soon anyway, we do not need to do anything here.
		showCursor = v;
	}

	Bool Frame::cursorVisible() {
		return showCursor;
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
		oldStyle = GetWindowLong(wnd, GWL_STYLE);
		RECT oldRect;
		GetWindowRect(wnd, &oldRect);
		oldPos = convert(oldRect);

		HMONITOR m = MonitorFromWindow(wnd, MONITOR_DEFAULTTONEAREST);
		MONITORINFO mi = { sizeof(mi) };
		GetMonitorInfo(m, &mi);

		SetWindowLong(wnd, GWL_STYLE, oldStyle & ~(WS_CAPTION | WS_THICKFRAME));

		LONG l = mi.rcMonitor.left;
		LONG r = mi.rcMonitor.right;
		LONG t = mi.rcMonitor.top;
		LONG b = mi.rcMonitor.bottom;
		SetWindowPos(wnd, NULL, l, t, r - l, b - t, SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
	}

	static void goBack(Handle wnd, Long &style, Rect &rect) {
		LONG l = LONG(rect.p0.x);
		LONG r = LONG(rect.p1.x);
		LONG t = LONG(rect.p0.y);
		LONG b = LONG(rect.p1.y);

		SetWindowLong(wnd, GWL_STYLE, LONG(style));
		SetWindowPos(wnd, NULL, l, t, r - l, b - t, SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
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
			ShowWindow(handle(), TRUE);
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

#endif

#ifdef GUI_GTK
	static void goFull(Handle wnd, Long &oldStyle, Rect &oldPos) {
		TODO(L"Fixme!");
	}

	static void goBack(Handle wnd, Long &oldStyle, Rect &oldPos) {
		TODO(L"Fixme!");
	}

	bool Frame::createWindow(bool sizeable) {
		GtkWidget *frame = gtk_window_new(GTK_WINDOW_TOPLEVEL);
		gtk_window_set_title((GtkWindow *)frame, text()->utf8_str());
		gtk_window_set_resizable((GtkWindow *)frame, sizeable);

		PVAR(basic_new());

		GtkWidget *container = gtk_fixed_new();
		gtk_container_add(GTK_CONTAINER(frame), container);
		gtk_widget_show(container);

		Signal<void, Frame>::Connect<&Frame::close>::to(frame, "destroy", engine());

		Size sz = pos().size();
		if (sz.w > 0 && sz.h > 0)
			gtk_window_set_default_size((GtkWindow *)frame, sz.w, sz.h);

		handle(frame);

		// Create child windows (if any).
		parentCreated(0);

		// Set visibility and full screen attributes.
		if (visible()) {
			gtk_widget_show(frame);
		}

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

	GtkFixed *Frame::container() {
		// There is a GTK_FIXED inside the frame.
		return GTK_FIXED(gtk_bin_get_child(GTK_BIN(handle().widget())));
	}

#endif

}
