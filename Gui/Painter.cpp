#include "stdafx.h"
#include "Painter.h"
#include "RenderResource.h"
#include "App.h"

namespace gui {

	Painter::Painter() : continuous(false), rendering(false), resized(false), repaintCounter(0), currentRepaint(0) {
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
			// Do not attempt to resize the drawing surface if we're currently drawing to it.
			while (rendering)
				os::UThread::leave();

			// This seems to not be neccessary. Probably because the resources are actually
			// associated to the underlying D3D device, and not the RenderTarget.
			// If the resize call fails for some reason, this is probably the cause.
			// destroyResources();
			RenderMgr *mgr = renderMgr(engine());
			mgr->resize(target, sz);
			if (graphics)
				graphics->updateTarget(target);

			// Note that we need to redraw before we have any valid contents.
			resized = true;
			mgr->painterReady();
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
			waitForFrame();
		} else {
			doRepaint(false);
		}

		currentRepaint = repaintCounter;
		afterRepaint(params);
	}

	void Painter::doRepaint(bool waitForVSync) {
		if (!target.any())
			return;

		bool more = false;
		try {
			rendering = true;
			more = doRepaintI(waitForVSync);
			rendering = false;
			resized = false;
		} catch (...) {
			repaintCounter++;
			rendering = false;
			resized = false;
			throw;
		}

		repaintCounter++;
		if (more != continuous) {
			continuous = more;
			if (more) {
				// Register!
				RenderMgr *mgr = renderMgr(engine());
				mgr->painterReady();
			}
		}
	}

#ifdef GUI_WIN32

	bool Painter::ready() {
		// Always ready to render!
		return true;
	}

	void Painter::waitForFrame() {
		// Wait until we have rendered a frame, so that Windows think we did it to satisfy the paint
		// request.
		nat old = repaintCounter;
		while (old == repaintCounter)
			os::UThread::leave();
	}

	void Painter::doRepaintI(bool waitForVSync) {
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

	bool Painter::ready() {
		// Do not try to repaint a new frame until the previous one has been displayed, or if we need a repaint.
		return repaintCounter == currentRepaint || resized;
	}

	void Painter::waitForFrame() {
		// It is fine to copy the contents of our buffer as long as we're not currently rendering to it.
		while (rendering || resized) {
			os::UThread::leave();
		}

		// Now we're ready for another frame. Let the RenderMgr know that 'ready' has changed.
		RenderMgr *mgr = renderMgr(engine());
		mgr->painterReady();
	}

	bool Painter::doRepaintI(bool waitForVSync) {
		if (!target.any())
			return continuous;

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

		// Notify Gtk+ that we have new content!
		if (waitForVSync) {
			// Note: It is safe to call 'app' from the Render thread here since we know that App has
			// been created previously (otherwise, we would not have been attached anywhere).
			App *a = app(engine());
			a->repaint(attachedTo);
		}

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

			resized = true;

			// Notify the RenderMgr, so that it knows we want to repaint now, in case we were done just before.
			RenderMgr *mgr = renderMgr(engine());
			mgr->painterReady();
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
