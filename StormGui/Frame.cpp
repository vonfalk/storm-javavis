#include "stdafx.h"
#include "Frame.h"
#include "App.h"

namespace stormgui {

	// Convert a client size to the actual window size.
	static Size clientToWindow(Size s, DWORD windowStyles, DWORD windowExStyles) {
		RECT r = { 0, 0, (LONG)s.w, (LONG)s.h };
		AdjustWindowRectEx(&r, windowStyles & ~WS_OVERLAPPED, FALSE, windowExStyles);
		return Size(Float(r.right - r.left), Float(r.bottom - r.top));
	}

	// Make 'wnd' go fullscreen.
	static void goFull(HWND wnd, FrameInfo &to) {
		to.style = GetWindowLong(wnd, GWL_STYLE);
		GetWindowRect(wnd, &to.rect);

		HMONITOR m = MonitorFromWindow(wnd, MONITOR_DEFAULTTONEAREST);
		MONITORINFO mi = { sizeof(mi) };
		GetMonitorInfo(m, &mi);

		SetWindowLong(wnd, GWL_STYLE, to.style & ~(WS_CAPTION | WS_THICKFRAME));

		LONG l = mi.rcMonitor.left;
		LONG r = mi.rcMonitor.right;
		LONG t = mi.rcMonitor.top;
		LONG b = mi.rcMonitor.bottom;
		SetWindowPos(wnd, NULL, l, t, r - l, b - t, SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
	}

	static void goBack(HWND wnd, FrameInfo &to) {
		LONG l = to.rect.left;
		LONG r = to.rect.right;
		LONG t = to.rect.top;
		LONG b = to.rect.bottom;

		SetWindowLong(wnd, GWL_STYLE, to.style);
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
			goFull(handle(), info);
		}

		return true;
	}

	Frame::Frame(Par<Str> title) : full(false) {
		attachParent(this);
		text(title);
		pos(Rect(-10000, -10000, -10002, -10002));
	}

	void Frame::create() {
		if (handle() != invalid)
			return;

		if (createWindow(true)) {
			// Add an extra reference to keep ourselves alive until we're closed.
			addRef();
		}
	}

	void Frame::close() {
		if (handle() != invalid) {
			handle(invalid);
			release();
		}

		onClose.set();
	}

	void Frame::waitForClose() {
		Auto<App> app = stormgui::app(engine());
		app->waitForEvent(this, onClose);
	}

	MsgResult Frame::onMessage(const Message &msg) {
		switch (msg.msg) {
		case WM_CLOSE:
			close();
			return msgResult(0);
		}

		return Container::onMessage(msg);
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
				goFull(handle(), info);
			} else {
				goBack(handle(), info);
			}
		}

		full = v;
	}

	Bool Frame::fullscreen() {
		return full;
	}

}
