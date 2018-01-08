#include "stdafx.h"
#include "Painter.h"
#include "RenderResource.h"
#include "App.h"

namespace gui {

	Painter::Painter() : continuous(false), rendering(false), repaintCounter(0), currentRepaint(0) {
		attachedTo = Window::invalid;
		app = gui::app(engine());
		bgColor = app->defaultBgColor;
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
		}
	}

	void Painter::create() {
		RenderMgr *mgr = renderMgr(engine());
		target = mgr->attach(this, attachedTo);
		graphics = new (this) Graphics(target, this);
	}

	void Painter::destroy() {
		// Wait until we're not rendering anymore.
		while (rendering)
			os::UThread::leave();

		// Go ahead and destroy all resources associated with this painter.
		destroyResources();

		if (graphics)
			graphics->destroyed();
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
		if (!target.any())
			return;

		if (continuous) {
			waitForFrame();
		} else {
			doRepaint(false, false);
		}

		currentRepaint = repaintCounter;
	}

	void Painter::repaintUi(RepaintParams *params) {
		beforeRepaint(params);

		if (!ready()) {
			// There is a frame ready right now! No need to wait even if we're not in continuous mode!
		} else if (continuous) {
			waitForFrame();
		} else {
			doRepaint(false, true);
		}

		currentRepaint = repaintCounter;
		afterRepaint(params);
	}

	void Painter::doRepaint(bool waitForVSync, bool fromWindow) {
		if (!target.any())
			return;

		bool more = false;
		try {
			rendering = true;
			more = doRepaintI(waitForVSync, fromWindow);
			rendering = false;
		} catch (...) {
			repaintCounter++;
			rendering = false;
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
		// Always ready to draw!
		return true;
	}

	void Painter::waitForFrame() {
		// Wait until we have rendered a frame, so that Windows think we did it to satisfy the paint
		// request.
		Nat old = repaintCounter;
		while (old == repaintCounter)
			os::UThread::leave();
	}

	void Painter::doRepaintI(bool waitForVSync, bool fromWindow) {
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

#endif
#ifdef GUI_GTK

	void Painter::waitForFrame() {
		// Just wait until we're not drawing at the moment.
		while (rendering)
			os::UThread::leave();
	}

	bool Painter::ready() {
		// Wait until the previous frame is actually shown.
		return currentRepaint == repaintCounter;
	}

	bool Painter::doRepaintI(bool waitForVSync, bool fromWindow) {
		if (!target.any())
			return continuous;

		bool more = false;

		// Clear the surface with the background color.
		cairo_set_source_rgba(target.target(), bgColor.r, bgColor.g, bgColor.b, bgColor.a);
		cairo_set_operator(target.target(), CAIRO_OPERATOR_SOURCE);
		cairo_paint(target.target());

		// Set the default operator again.
		cairo_set_operator(target.target(), CAIRO_OPERATOR_OVER);

		try {
			graphics->beforeRender();
			more = render(graphics->size(), graphics);
			graphics->afterRender();
		} catch (...) {
			graphics->afterRender();
			throw;
		}

		cairo_surface_flush(target.surface()->cairo);

		// TODO: Handle VSync?
		// Show the result if we're in continuous mode.
		if (fromWindow)
			app->repaint(attachedTo);

		return more;
	}

	void Painter::beforeRepaint(RepaintParams *p) {
		if (!target.any()) {
			// We can create the actual context now that we know we have a window to draw to.
			RenderMgr *mgr = renderMgr(engine());
			target = mgr->create(p->widget, p->target);

			if (graphics)
				graphics->updateTarget(target);
		}
	}

	void Painter::afterRepaint(RepaintParams *p) {
		// Show the frame. We need to do this in sync with the Gtk+ thread, otherwise we will cause havoc!
		if (GlSurface *surface = target.surface()) {
			surface->swapBuffers();

			if (continuous) {
				RenderMgr *mgr = renderMgr(engine());
				mgr->painterReady();
			}
		}
	}

#endif
}
