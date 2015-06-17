#include "stdafx.h"
#include "Painter.h"
#include "Window.h"
#include "RenderMgr.h"

namespace stormgui {

	Painter::Painter() {
		attachedTo = Window::invalid;
	}

	Painter::~Painter() {
		detach();
	}

	void Painter::resized(Size size) {
	}

	Bool Painter::render() {
		return false;
	}

	void Painter::attach(Par<Window> to) {
		HWND handle = to->handle();
		if (handle != attachedTo) {
			size = to->pos().size();
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

	void Painter::resize(Size size) {
		if (target) {
			D2D1_SIZE_U s = { size.w, size.h };
			target->Resize(s);
		}

		resized(size);
	}

	void Painter::create() {
		Auto<RenderMgr> mgr = renderMgr(engine());
		target = mgr->attach(this, attachedTo);
	}

	void Painter::destroy() {
		::release(target);

		Auto<RenderMgr> mgr = renderMgr(engine());
		mgr->detach(this);
	}

	void Painter::repaint() {
		if (!target)
			return;

		target->BeginDraw();
		D2D1_COLOR_F c = { 1.0f, 1.0f, 0.0f };
		target->Clear(c);
		target->EndDraw();
	}

}
