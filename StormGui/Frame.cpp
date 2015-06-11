#include "stdafx.h"
#include "Frame.h"
#include "App.h"

namespace stormgui {

	// Convert a client size to the actual window size.
	static Size clientToWindow(Size s, DWORD windowStyles, DWORD windowExStyles) {
		RECT r = { 0, 0, s.w, s.h };
		AdjustWindowRectEx(&r, windowStyles & ~WS_OVERLAPPED, FALSE, windowExStyles);
		return Size(r.right - r.left, r.bottom - r.top);
	}

	// Helper to create the actual handle.
	bool Frame::createWindow(const String &caption, Size size, bool sizeable) {
		Auto<App> app = stormgui::app(engine());

		DWORD exStyles = WS_EX_CONTROLPARENT;
		DWORD styles = 0;
		if (sizeable)
			styles = WS_OVERLAPPEDWINDOW;
		else
			styles = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;

		styles |= WS_CLIPCHILDREN;

		nat w = CW_USEDEFAULT;
		nat h = 0;
		if (size.w > 0 && size.h > 0) {
			size = clientToWindow(size, styles, exStyles);
			w = size.w;
			h = size.h;
		}

		return createEx(exStyles,
						(LPCTSTR)app->windowClass(),
						caption.c_str(),
						styles,
						CW_USEDEFAULT, 0,
						w, h,
						NULL,
						NULL);
	}

	Frame::Frame() {}

	void Frame::create(Par<Str> title) {
		create(title, Size());
	}

	void Frame::create(Par<Str> title, Size size) {
		if (handle() != invalid)
			return;

		if (createWindow(title->v, size, true)) {
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

		return Window::onMessage(msg);
	}

}
