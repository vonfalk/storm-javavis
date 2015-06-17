#include "stdafx.h"
#include "Painter.h"
#include "Window.h"
#include "RenderMgr.h"
#include "RenderResource.h"

namespace stormgui {

	Painter::Painter(Bool continuous) {
		attachedTo = Window::invalid;
		bgColor = color(GetSysColor(COLOR_3DFACE));

		if (continuous) {
			WARNING(L"Continuous updates are not yet supported.");
			TODO(L"Implement using another kind of render target (DXGI)");
		}
	}

	Painter::~Painter() {
		for (hash_set<RenderResource *>::iterator i = resources.begin(), end = resources.end(); i != end; ++i)
			(*i)->forgetOwner();

		detach();
	}

	void Painter::render(Size size, Par<Graphics> graphics) {}

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

	void Painter::resize(Size sz) {
		if (target) {
			D2D1_SIZE_U s = { (UINT32)sz.w, (UINT32)sz.h };
			target->Resize(s);
		}
	}

	void Painter::create() {
		Auto<RenderMgr> mgr = renderMgr(engine());
		target = mgr->attach(this, attachedTo);
		graphics = CREATE(Graphics, this, target, this);
	}

	void Painter::destroy() {
		for (hash_set<RenderResource *>::iterator i = resources.begin(), end = resources.end(); i != end; ++i)
			(*i)->destroy();

		graphics = null;
		::release(target);

		Auto<RenderMgr> mgr = renderMgr(engine());
		mgr->detach(this);
	}

	void Painter::addResource(Par<RenderResource> r) {
		resources.insert(r.borrow());
	}

	void Painter::removeResource(Par<RenderResource> r) {
		resources.erase(r.borrow());
	}

	void Painter::repaint() {
		if (!target)
			return;

		target->BeginDraw();
		target->SetTransform(D2D1::Matrix3x2F::Identity());
		target->Clear(dx(bgColor));

		try {
			render(graphics->size(), graphics);
		} catch (...) {
			target->EndDraw();
			throw;
		}

		HRESULT r = target->EndDraw();
		if (r == D2DERR_RECREATE_TARGET) {
			// Re-create our render target.
			destroy();
			create();
		}
	}

}
