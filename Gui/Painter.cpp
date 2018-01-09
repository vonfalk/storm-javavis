#include "stdafx.h"
#include "Painter.h"
#include "RenderResource.h"
#include "App.h"

namespace gui {

	Painter::Painter() : continuous(false), rendering(false), repaintCounter(0), currentRepaint(0) {
		attachedTo = Window::invalid;
		app = gui::app(engine());
		bgColor = app->defaultBgColor;
		mgr = gui::renderMgr(engine());
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

			mgr->resize(target, sz);
			if (graphics)
				graphics->updateTarget(target);
		}
	}

	void Painter::create() {
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
		// TODO: FIXME! Does not currently work for single-threaded usage.
		if (!target.any())
			return;

		if (continuous) {
			waitForFrame();
		} else {
			doRepaint(false);
		}

		afterRepaint();
	}

	void Painter::doRepaint(bool waitForVSync) {
		if (!target.any())
			return;

		bool more = false;
		try {
			rendering = true;
			more = doRepaintI(waitForVSync);
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
				mgr->painterReady();
			}
		}
	}

#ifdef SINGLE_THREADED_UI

	void Painter::repaintI(RepaintParams *params) {
		beforeRepaint(params);

		// Nothing is done in parallell: just draw the frame right now!
		doRepaint(false);

		afterRepaint();
	}

	void Painter::uiAttach(Window *to) {
		attach(to);
	}

	void Painter::uiDetach() {
		detach();
	}

	void Painter::uiResize(Size size) {
		resize(size);
	}

	void Painter::uiRepaint(RepaintParams *par) {
		repaintI(par);
		uiAfterRepaint(par);
	}

#else

	void Painter::repaintI(RepaintParams *params) {
		beforeRepaint(params);

		if (!ready()) {
			// There is a frame ready right now! No need to wait even if we're not in continuous mode!
		} else if (continuous) {
			waitForFrame();
		} else {
			doRepaint(false, true);
		}

		afterRepaint();
	}

	void Painter::uiAttach(Window *to) {
		os::Future<void> result;
		Painter *me = this;
		os::FnCall<void, 2> params = os::fnCall().add(me).add(to);
		os::UThread::spawn(address(&Painter::attach), true, params, result, &thread->thread());
		result.result();
	}

	void Painter::uiDetach() {
		os::Future<void> result;
		Painter *me = this;
		os::FnCall<void, 2> params = os::fnCall().add(me);
		os::UThread::spawn(address(&Painter::detach), true, params, result, &thread->thread());
		result.result();
	}

	void Painter::uiResize(Size size) {
		os::Future<void> result;
		Painter *me = this;
		os::FnCall<void, 2> params = os::fnCall().add(me).add(size);
		os::UThread::spawn(address(&Painter::resize), true, params, result, &thread->thread());
		result.result();
	}

	void Painter::uiRepaint(RepaintParams *par) {
		os::Future<void> result;
		Painter *me = this;
		os::FnCall<void, 2> params = os::fnCall().add(me).add(par);
		os::UThread::spawn(address(&Painter::repaintI), true, params, result, &thread->thread());
		result.result();

		uiAfterRepaint();
	}

#endif
#ifdef GUI_WIN32

#ifdef SINGLE_THREADED_UI
#error "Single threaded UI is not supported on Win32! Please disable SINGLE_THREADED_UI in stdafx.h."
#endif

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

	void Painter::afterRepaint() {
		currentRepaint = repaintCounter;
	}

	void Painter::uiAfterRepaint() {}

#endif
#ifdef GUI_GTK

#ifndef SINGLE_THREADED_UI
#error "Multithreading with Gtk+ is not yet supported."
#endif

	void Painter::waitForFrame() {
		// Just wait until we're not drawing at the moment.
		while (rendering)
			os::UThread::leave();
	}

	bool Painter::ready() {
		// Wait until the previous frame is actually shown.
		return atomicRead(currentRepaint) == atomicRead(repaintCounter);
	}

	bool Painter::doRepaintI(bool waitForVSync) {
		if (!target.any())
			return continuous;

		bool more = false;

		GlSurface *surface = target.surface();
		cairo_surface_mark_dirty(surface->cairo);

		// Clear the surface with the background color.
		cairo_set_source_rgba(target.target(), bgColor.r, bgColor.g, bgColor.b, bgColor.a);
		cairo_set_operator(target.target(), CAIRO_OPERATOR_SOURCE);
		cairo_paint(target.target());

		// Set the the defaults once again, just in case.
		cairo_set_operator(target.target(), CAIRO_OPERATOR_OVER);
		cairo_set_fill_rule(target.target(), CAIRO_FILL_RULE_EVEN_ODD);

		try {
			graphics->beforeRender();
			more = render(graphics->size(), graphics);
			graphics->afterRender();
		} catch (...) {
			graphics->afterRender();
			throw;
		}

		cairo_surface_flush(surface->cairo);

		// TODO: Handle VSync?
		return more;
	}

	void Painter::beforeRepaint(RepaintParams *p) {
		if (!target.any()) {
			// We can create the actual context now that we know we have a window to draw to.
			target = mgr->create(p->widget, p->target);

			if (graphics)
				graphics->updateTarget(target);
		}
	}

	void Painter::afterRepaint() {}

	void Painter::uiAfterRepaint(RepaintParams *params) {
		currentRepaint = repaintCounter;

		if (GlSurface *surface = target.surface()) {
#ifdef GTK_RENDER_SINGLE_GL_COPY
			cairo_set_source_surface(params->ctx, surface->cairo, 0, 0);
			cairo_paint(params->ctx);
#else
			surface->swapBuffers();
#endif

			// We're ready for the next frame now!
			if (continuous) {
				mgr->painterReady();
				if (GdkWindow *window = gtk_widget_get_window(attachedTo.widget()))
					gdk_window_invalidate_rect(window, NULL, true);
			}
		}
	}

#endif
}
