#include "stdafx.h"
#include "Painter.h"
#include "RenderResource.h"

namespace gui {

	Painter::Painter() : continuous(false), repaintCounter(0) {
		attachedTo = Window::invalid;
		TODO(L"Find default BG color!");
		// bgColor = color(GetSysColor(COLOR_3DFACE));
		resources = new (this) WeakSet<RenderResource>();
	}

	Painter::~Painter() {
		WeakSet<RenderResource>::Iter i = resources->iter();
		while (RenderResource *r = i.next())
			r->forgetOwner();

		detach();
	}

	Bool Painter::render(Size size, Graphics *graphics) {
		return false;
	}

	void Painter::attach(Window *to) {
		Handle handle = to->handle();
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
		if (target.any()) {
			// This seems to not be neccessary. Probably because the resources are actually
			// associated to the underlying D3D device, and not the RenderTarget.
			// If the resize call fails for some reason, this is probably the cause.
			// destroyResources();
			RenderMgr *mgr = renderMgr(engine());
			mgr->resize(target, sz);
			if (graphics)
				graphics->updateTarget(target);
		}
	}

	void Painter::create() {
		RenderMgr *mgr = renderMgr(engine());
		target = mgr->attach(this, attachedTo);
		graphics = new (this) Graphics(target, this);
	}

	void Painter::destroy() {
		destroyResources();

		graphics = null;
		target.release();

		RenderMgr *mgr = renderMgr(engine());
		mgr->detach(this);
	}

	void Painter::destroyResources() {
		WeakSet<RenderResource>::Iter i = resources->iter();
		while (RenderResource *r = i.next())
			r->destroy();
	}

	void Painter::addResource(RenderResource *r) {
		resources->put(r);
	}

	void Painter::removeResource(RenderResource *r) {
		resources->remove(r);
	}

	void Painter::repaint() {
		if (continuous) {
			nat old = repaintCounter;
			while (old == repaintCounter)
				os::UThread::leave();
		} else {
			doRepaint(false);
		}

		// TODO: On Gtk+, we need to copy our buffer to the window here.
	}

	void Painter::doRepaint(bool waitForVSync) {
		if (!target.any())
			return;

		bool more = false;
		try {
			more = doRepaintI(waitForVSync);
		} catch (...) {
			repaintCounter++;
			throw;
		}

		repaintCounter++;
		if (more != continuous) {
			continuous = more;
			if (more) {
				// Register!
				RenderMgr *mgr = renderMgr(engine());
				mgr->newContinuous();
			}
		}
	}

#ifdef GUI_WIN32

	void Painter::doRepaint(bool waitForVSync) {
		if (!target.target())
			return false;
		if (!target.swapChain())
			return false;

		target.target()->BeginDraw();
		target.target()->SetTransform(D2D1::Matrix3x2F::Identity());
		target.target()->Clear(dx(bgColor));

		bool more = false;

		try {
			graphics->beforeRender();
			more = render(graphics->size(), graphics);
			graphics->afterRender();
		} catch (...) {
			graphics->afterRender();
			// target.target()->EndDraw();
			throw;
		}

		HRESULT r = target.target()->EndDraw();

		if (SUCCEEDED(r)) {
			if (waitForVSync) {
				r = target.swapChain()->Present(1, 0);
			} else {
				r = target.swapChain()->Present(0, 0);
			}
		}

		if (r == D2DERR_RECREATE_TARGET || r == DXGI_ERROR_DEVICE_RESET) {
			// Re-create our render target.
			destroy();
			create();
			// TODO: We probably want to re-draw ourselves here...
		}

		return more;
	}

#endif
#ifdef GUI_GTK

	bool Painter::doRepaintI(bool waitForVSync) {
		if (!target.any())
			return false;

		try {
			graphics->beforeRender();
			render(graphics->size(), graphics);
			graphics->afterRender();
		} catch (...) {
			graphics->afterRender();
			throw;
		}

		return false;
	}

#endif
}
