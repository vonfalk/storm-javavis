#include "stdafx.h"
#include "Painter.h"
#include "RenderResource.h"

namespace gui {

	Painter::Painter() : continuous(false), repaintCounter(0) {
		attachedTo = Window::invalid;
		bgColor = getBgColor();
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

	void Painter::repaint(RepaintParams *params) {
		beforeRepaint(params);

		if (continuous) {
			nat old = repaintCounter;
			while (old == repaintCounter)
				os::UThread::leave();
		} else {
			doRepaint(false);
		}

		afterRepaint(params);
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

	void Painter::beforeRepaint(RepaintParams *handle) {}

	void Painter::afterRepaint(RepaintParams *handle) {}

	Color Painter::getBgColor() {
		return color(GetSysColor(COLOR_3DFACE));
	}

#endif
#ifdef GUI_GTK

	bool Painter::doRepaintI(bool waitForVSync) {
		if (!target.any())
			return false;

		bool more = false;

		// Clear the surface with the desired color. TODO: Set the clip properly?
		// TODO: We probably want to save the background color in a cairo_pattern_t and use that.
		cairo_set_source_rgba(target.device(), bgColor.r, bgColor.g, bgColor.b, bgColor.a);
		cairo_set_operator(target.device(), CAIRO_OPERATOR_SOURCE);
		cairo_paint(target.device());

		// Restore the default operator.
		cairo_set_operator(target.device(), CAIRO_OPERATOR_OVER);

		try {
			graphics->beforeRender();
			more = render(graphics->size(), graphics);
			graphics->afterRender();
		} catch (...) {
			graphics->afterRender();
			throw;
		}

		// Flush any pending operations.
		cairo_surface_flush(target.surface());

		// TODO: If 'waitForVSync' is true: we shall call 'repaint' (or 'invalidate') on the window
		// and wait until we get a callback from that window. Otherwise, animations won't work.

		return more;
	}

	void Painter::beforeRepaint(RepaintParams *p) {
		if (!target.any()) {
			// Update the size, since it might have changed since we last observed it.
			Size sz = Size(gtk_widget_get_allocated_width(p->widget),
						gtk_widget_get_allocated_height(p->widget));
			target.size = sz;

			// We should create the target!
			cairo_surface_t *realTarget = cairo_get_target(p->target);
			target.surface(cairo_surface_create_similar(realTarget, CAIRO_CONTENT_COLOR_ALPHA, sz.w, sz.h));
			target.device(cairo_create(target.surface()));

			if (graphics)
				graphics->updateTarget(target);
		}
	}

	void Painter::afterRepaint(RepaintParams *p) {
		if (!target.any())
			return;

		cairo_t *to = p->target;
		cairo_set_source_surface(to, target.surface(), 0, 0);
		cairo_paint(to);
	}

	Color Painter::getBgColor() {
		// Transparent.
		return Color(0.0f, 0.0f, 0.0f, 0.0f);
	}

#endif
}
