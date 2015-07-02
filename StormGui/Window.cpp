#include "stdafx.h"
#include "Window.h"
#include "Frame.h"
#include "App.h"
#include "Painter.h"

namespace stormgui {

	// Null is a failure code from eg. GetParent function, should be OK to use as invalid.
	const HWND Window::invalid = (HWND)NULL;

	Window::Window() : myHandle(invalid), myParent(null), myRoot(null), myVisible(true), myPos(0, 0, 10, 10) {
		myFont = steal(app(engine()))->defaultFont;
	}

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
		if (parent == this)
			myRoot = as<Frame>(this);
		else
			myRoot = parent->myRoot;
	}

	void Window::parentCreated(nat id) {
		assert(myParent);

		myRoot = myParent->myRoot;

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
			attachPainter();
		} else {
			detachPainter();
		}
	}

	MsgResult Window::onMessage(const Message &msg) {
		switch (msg.msg) {
		case WM_SIZE: {
			Size s = pos().size();
			notifyPainter(s);
			resized(s);
			return msgResult(0);
		}
		case WM_PAINT:
			return onPaint();
		}
		return noResult();
	}

	MsgResult Window::beforeMessage(const Message &msg) {
		switch (msg.msg) {
		case WM_KEYUP:
			if (onKey(false, msg.wParam))
				return msgResult(0);
			break;
		case WM_KEYDOWN:
			if (onKey(true, msg.wParam))
				return msgResult(0);
			break;
		case WM_CHAR:
			if (onChar(msg.wParam))
				return msgResult(0);
			break;
		}
		return noResult();
	}

	Bool Window::onKey(Bool down, Nat id) {
		return false;
	}

	Bool Window::onChar(Nat id) {
		return false;
	}

	bool Window::onCommand(nat id) {
		return false;
	}

	Bool Window::visible() {
		return myVisible;
	}

	void Window::visible(Bool show) {
		if (created())
			ShowWindow(handle(), show ? TRUE : FALSE);
	}

	Str *Window::text() {
		return CREATE(Str, this, cText());
	}

	void Window::text(Par<Str> s) {
		cText(s->v);
	}

	const String &Window::cText() {
		if (created()) {
			int len = GetWindowTextLength(handle());
			wchar *buffer = new wchar[len + 1];
			GetWindowText(handle(), buffer, len + 1);
			myText = buffer;
			delete []buffer;
		}
		return myText;
	}

	void Window::cText(const String &s) {
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

	Font *Window::font() {
		return CREATE(Font, this, myFont);
	}

	void Window::font(Par<Font> font) {
		myFont = CREATE(Font, this, font);
		if (created())
			SendMessage(handle(), WM_SETFONT, (WPARAM)font->handle(), TRUE);
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
			Rect c = convert(r);
			p = c.p0;
			s = c.size();
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
								(int)p.x, (int)p.y, (int)s.w, (int)s.h,
								parent, (HMENU)id, instance, NULL);

		if (z == NULL) {
			app->createAborted(this);
			return false;
		} else {
			handle(z);
			SendMessage(handle(), WM_SETFONT, (WPARAM)myFont->handle(), TRUE);
			return true;
		}
	}

	void Window::painter(Par<Painter> p) {
		Engine &e = engine();
		detachPainter();

		myPainter = p;

		if (created())
			attachPainter();
	}

	void Window::attachPainter() {
		if (myPainter) {
			Engine &e = engine();
			os::Future<void> result;
			os::FnParams params;
			params.add(myPainter.borrow()).add(this);
			os::UThread::spawn(address(&Painter::attach), true, params, result, &Render::thread(e)->thread());
			result.result();
		}
	}

	void Window::detachPainter() {
		if (myPainter) {
			Engine &e = engine();
			os::Future<void> result;
			os::FnParams params;
			params.add(myPainter.borrow());
			os::UThread::spawn(address(&Painter::detach), true, params, result, &Render::thread(e)->thread());
			result.result();
		}
	}

	void Window::notifyPainter(Size s) {
		if (myPainter) {
			Engine &e = engine();
			os::FnParams params; params.add(myPainter.borrow()).add(s);
			os::UThread::spawn(address(&Painter::resize), true, params, &Render::thread(e)->thread());
		}
	}

	MsgResult Window::onPaint() {
		if (myPainter) {
			Engine &e = engine();
			os::FnParams params; params.add(myPainter.borrow());
			os::UThread::spawn(address(&Painter::repaint), true, params, &Render::thread(e)->thread());

			// Tell Windows we've taken care of re-painting our window (we will soon, at least...)
			ValidateRect(handle(), NULL);

			return msgResult(0);
		} else {
			return noResult();
		}
	}

}
