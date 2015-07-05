#include "stdafx.h"
#include "Painter.h"
#include "Window.h"
#include "RenderMgr.h"
#include "RenderResource.h"

namespace stormgui {

	Painter::Painter() : continuous(false), repaintCounter(0) {
		attachedTo = Window::invalid;
		bgColor = color(GetSysColor(COLOR_3DFACE));
	}

	Painter::~Painter() {
		for (hash_set<RenderResource *>::iterator i = resources.begin(), end = resources.end(); i != end; ++i)
			(*i)->forgetOwner();

		detach();
	}

	Bool Painter::render(Size size, Par<Graphics> graphics) {
		return false;
	}

	void Painter::attach(Par<Window> to) {
		HWND handle = to->handle();
		if (handle != attachedTo) {
			detach();
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
		if (target.swapChain) {
			// This seems to not be neccessary. Probably because the resources are actually
			// associated to the underlying D3D device, and not the RenderTarget.
			// If the resize call fails for some reason, this is probably the cause.
			// destroyResources();
			Auto<RenderMgr> mgr = renderMgr(engine());
			mgr->resize(target, sz);
			if (graphics)
				graphics->updateTarget(target.target);
		}
	}

	void Painter::create() {
		Auto<RenderMgr> mgr = renderMgr(engine());
		target = mgr->attach(this, attachedTo);
		graphics = CREATE(Graphics, this, target.target, this);
	}

	void Painter::destroy() {
		destroyResources();

		graphics = null;
		target.release();

		Auto<RenderMgr> mgr = renderMgr(engine());
		mgr->detach(this);
	}

	void Painter::destroyResources() {
		for (hash_set<RenderResource *>::iterator i = resources.begin(), end = resources.end(); i != end; ++i)
			(*i)->destroy();
	}

	void Painter::addResource(Par<RenderResource> r) {
		resources.insert(r.borrow());
	}

	void Painter::removeResource(Par<RenderResource> r) {
		resources.erase(r.borrow());
	}

	void Painter::repaint() {
		if (continuous) {
			nat old = repaintCounter;
			while (old == repaintCounter)
				os::UThread::leave();
		} else {
			doRepaint(false);
		}
	}

	void Painter::doRepaint(bool waitForVSync) {
		if (!target.target)
			return;
		if (!target.swapChain)
			return;

		target.target->BeginDraw();
		target.target->SetTransform(D2D1::Matrix3x2F::Identity());
		target.target->Clear(dx(bgColor));

		bool more = false;

		try {
			graphics->reset();
			more = render(graphics->size(), graphics);
		} catch (...) {
			target.target->EndDraw();
			repaintCounter++;
			throw;
		}

		HRESULT r = target.target->EndDraw();

		if (SUCCEEDED(r)) {
			if (waitForVSync) {
				r = target.swapChain->Present(1, 0);
			} else {
				r = target.swapChain->Present(0, 0);
			}
		}

		if (r == D2DERR_RECREATE_TARGET || r == DXGI_ERROR_DEVICE_RESET) {
			// Re-create our render target.
			destroy();
			create();
			// We probably want to re-draw ourselves here...
		}

		repaintCounter++;

		if (more != continuous) {
			continuous = more;
			if (more) {
				// Register!
				Auto<RenderMgr> mgr = renderMgr(engine());
				mgr->newContinuous();
			}
		}
	}

}
