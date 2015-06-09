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

}
