#include "stdafx.h"
#include "Window.h"
#include "Frame.h"
#include "App.h"

namespace stormgui {

	// Null is a failure code from eg. GetParent function, should be OK to use as invalid.
	const HWND Window::invalid = (HWND)NULL;

	Window::Window() : myHandle(invalid), myParent(null), myRoot(null), myVisible(true), myPos(0, 0, 10, 10) {}

	Window::~Window() {
		if (myHandle != invalid) {
			Auto<App> a = app(engine());
			a->removeWindow(this);
		}
	}

	Container *Window::parent() {
		myParent->addRef();
		return myParent;
	}

	Frame *Window::rootFrame() {
		myRoot->addRef();
		return myRoot;
	}

	HWND Window::handle() const {
		return myHandle;
	}

	void Window::attachParent(Container *parent) {
		myParent = parent;
	}

	void Window::parentCreated(nat id) {
		if (!created())
			if (!create(myParent->handle(), id))
				WARNING(L"Failed to create a window...");
	}

	void Window::handle(HWND handle) {
		Auto<App> a = app(engine());
		if (myHandle != invalid) {
			// Sends WM_CLOSE, which may be handled by this class.
			DestroyWindow(myHandle);
			a->removeWindow(this);
		}

		myHandle = handle;
		if (myHandle != invalid) {
			a->addWindow(this);
		}
	}

	MsgResult Window::onMessage(const Message &msg) {
		switch (msg.msg) {
		case WM_SIZE:
			resized(pos().size());
			return msgResult(0);
		}
		return noResult();
	}

	Bool Window::visible() {
		return myVisible;
	}

	void Window::visible(Bool show) {
		if (created())
			ShowWindow(handle(), show ? TRUE : FALSE);
	}

	Str *Window::text() {
		if (created())
			TODO(L"Get the actual window text!");
		return CREATE(Str, this, myText);
	}

	void Window::text(Par<Str> s) {
		text(s->v);
	}

	void Window::text(const String &s) {
		myText = s;
		if (created())
			SetWindowText(handle(), s.c_str());
	}

	Rect Window::pos() {
		if (created()) {
			RECT r;
			HWND h = handle();
			GetClientRect(h, &r);
			POINT a = { r.left, r.top };
			ClientToScreen(h, &a);
			POINT b = { r.right, r. bottom };
			ClientToScreen(h, &b);
			myPos = convert(a, b);
		}
		return myPos;
	}

	void Window::pos(Rect r) {
		myPos = r;
		if (created()) {
			RECT z = convert(r);
			HWND h = handle();
			DWORD style = GetWindowLong(h, GWL_STYLE);
			DWORD exStyle = GetWindowLong(h, GWL_EXSTYLE);
			AdjustWindowRectEx(&z, style, FALSE, exStyle);
			// Todo: keep track if we need to repaint.
			MoveWindow(handle(), z.left, z.top, z.right - z.left, z.bottom - z.top, TRUE);
		}
	}

	void Window::update() {
		UpdateWindow(handle());
	}

	void Window::resized(Size size) {
		// Nothing here, override when needed.
	}

	bool Window::create(HWND parent, nat id) {
		return createEx(NULL, childFlags, 0, parent, id);
	}

	bool Window::createEx(LPCTSTR className, DWORD style, DWORD exStyle, HWND parent, nat id, CreateFlags flags) {
		assert(handle() == invalid);

		Auto<App> app = stormgui::app(engine());

		if (className == null)
			className = (LPCTSTR)app->windowClass();

		if (cManualVisibility & ~flags)
			if (myVisible)
				style |= WS_VISIBLE;

		Point p = myPos.p0;
		Size s = myPos.size();

		if (WS_CHILD & ~style) {
			RECT r = convert(myPos);
			AdjustWindowRectEx(&r, style & ~WS_OVERLAPPED, FALSE, exStyle);
			p.x = r.left; p.y = r.top;
			s.w = r.right - r.left; s.h = r.bottom - r.top;
		}

		if (flags & cAutoPos) {
			// Outside the screen?
			if (myPos.p1.x < 0 && myPos.p1.y < 0) {
				p.x = CW_USEDEFAULT;
				p.y = 0;
			}

			// Invalid size?
			if (!myPos.size().valid()) {
				s.w = CW_USEDEFAULT;
				s.h = 0;
			}
		}

		// Position controls before creation.
		if (myPos.size().valid())
			resized(myPos.size());

		HINSTANCE instance = app->instance();
		LPCTSTR windowName = myText.c_str();

		app->preCreate(this);
		HWND z = CreateWindowEx(exStyle, className, windowName, style,
								p.x, p.y, s.w, s.h,
								parent, (HMENU)id, instance, NULL);

		if (z == NULL) {
			app->createAborted(this);
			return false;
		} else {
			handle(z);
			return true;
		}
	}

}
