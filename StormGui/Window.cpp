#include "stdafx.h"
#include "Window.h"
#include "Frame.h"
#include "App.h"

namespace stormgui {

	// Null is a failure code from eg. GetParent function, should be OK to use as invalid.
	const HWND Window::invalid = (HWND)NULL;

	Window::Window() : myHandle(invalid), root(null) {}

	Window::~Window() {
		if (myHandle != invalid) {
			Auto<App> a = app(engine());
			a->removeWindow(this);
		}
	}

	Frame *Window::rootFrame() {
		root->addRef();
		return root;
	}

	HWND Window::handle() const {
		return myHandle;
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
			root = a->addWindow(this);
		}
	}

	MsgResult Window::onMessage(const Message &msg) {
		return noResult();
	}

	void Window::show(Bool show) {
		ShowWindow(handle(), show ? TRUE : FALSE);
	}

	void Window::show() {
		show(true);
	}

	void Window::initDone() {
		show();
		update();
	}

	void Window::update() {
		UpdateWindow(handle());
	}

	bool Window::create(LPCTSTR className, LPCTSTR windowName, DWORD style,
						int x, int y, int w, int h,
						HWND parent, HMENU menu) {
		assert(handle() == invalid);

		Auto<App> app = stormgui::app(engine());
		app->preCreate(this);
		HWND z = CreateWindow(className, windowName, style, x, y, w, h, parent, menu, app->instance(), NULL);

		if (z == NULL) {
			app->createAborted(this);
			return false;
		} else {
			handle(z);
			return true;
		}
	}

	bool Window::createEx(DWORD exStyle, LPCTSTR className, LPCTSTR windowName, DWORD style,
						int x, int y, int w, int h,
						HWND parent, HMENU menu) {
		assert(handle() == invalid);

		Auto<App> app = stormgui::app(engine());
		app->preCreate(this);
		HWND z = CreateWindowEx(exStyle, className, windowName, style, x, y, w, h, parent, menu, app->instance(), NULL);

		if (z == NULL) {
			app->createAborted(this);
			return false;
		} else {
			handle(z);
			return true;
		}
	}

}
