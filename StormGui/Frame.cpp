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

		return true;
	}

	Frame::Frame(Par<Str> title) {
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
		Rect p = pos();
		p.size(s);
		pos(p);
	}
}
