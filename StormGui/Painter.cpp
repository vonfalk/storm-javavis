#include "stdafx.h"
#include "Painter.h"
#include "Window.h"

namespace stormgui {

	Painter::Painter() {
		attachedTo = Window::invalid;
	}

	Painter::~Painter() {
		detach();
	}

	Bool Painter::render(Size size) {
		return false;
	}

	void Painter::attach(Par<Window> to) {
		HWND handle = to->handle();
		if (handle != attachedTo) {
			attachedTo = handle;
			create();
		}
	}

	void Painter::detach() {
		if (attachedTo != Window::invalid) {
			attachedTo = Window::invalid;
			destroy();
		}
	}

	void Painter::create() {}

	void Painter::destroy() {}

}
