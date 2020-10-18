#include "stdafx.h"
#include "Painter.h"
#include "RenderResource.h"
#include "App.h"

namespace gui {

	Painter::Painter() : continuous(false), repaintBuffer(0), repaintScreen(-1) {
		attachedTo = Window::invalid;
		app = gui::app(engine());
		bgColor = app->defaultBgColor;
		mgr = gui::renderMgr(engine());
		deviceType = mgr->deviceType();
		resources = new (this) WeakSet<RenderResource>();
		lock = new (this) Lock();
	}

	Painter::~Painter() {
		WeakSet<RenderResource>::Iter i = resources->iter();
		while (RenderResource *r = i.next())
			r->forgetOwner();

		onDetach();
	}

	Bool Painter::render(Size size, Graphics *graphics) {
		return false;
	}

	void Painter::onAttach(Window *to) {
		Handle handle = to->handle();
		if (handle != attachedTo) {
			onDetach();
			attachedTo = handle;
			create();
		}
	}

	void Painter::onDetach() {
		if (attachedTo != Window::invalid) {
			attachedTo = Window::invalid;
			destroy();
		}
	}

	void Painter::onResize(Size sz, Float scale) {
		if (target.any()) {
			// Do not attempt to resize the drawing surface if we're currently drawing to it.
			Lock::Guard z(lock);

			// This seems to not be neccessary. Probably because the resources are actually
			// associated to the underlying D3D device, and not the RenderTarget.
			// If the resize call fails for some reason, this is probably the cause.

			// destroyResources();

			mgr->resize(target, sz, scale);
			if (graphics)
				graphics->updateTarget(target);
		}
	}

	void Painter::create() {
		target = mgr->attach(this, attachedTo);
		graphics = new (this) WindowGraphics(target, this);
	}

	void Painter::destroy() {
		// Wait until we're not rendering anymore.
		Lock::Guard z(lock);

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
		if (!target.any())
			return;

		if (continuous) {
			waitForFrame();
			return;
		}

		switch (deviceType) {
		case dtRaw:
			// Just repaint. Don't wait for vsync.
			doRepaint();
			present(false);
			break;
		case dtBuffered:
			// Ask the windowing system for a repaint instead.
			invalidateWindow();
			break;
		}
	}

	void Painter::onRepaint(RepaintParams *params) {
		// Do we need to render the frame now, or did we already render something before?
		if (ready())
			doRepaint();

		// Present the image to the screen.
		present(true);
	}

	void Painter::doRepaint() {
		if (!target.any())
			return;

		bool more = false;
		try {
			Lock::Guard z(lock);
			more = doRepaintI();
		} catch (...) {
			atomicIncrement(repaintBuffer);
			throw;
		}

		atomicIncrement(repaintBuffer);
		if (more != continuous) {
			continuous = more;
			if (more) {
				switch (deviceType) {
				case dtRaw:
					// Register!
					mgr->painterReady();
					break;
				case dtBuffered:
					// Let the windowing system handle it.
					invalidateWindow();
					break;
				}
			}
		}
	}

	bool Painter::ready() {
		// Wait until the current frame is shown on the screen.
		return atomicRead(repaintBuffer) != atomicRead(repaintScreen);
	}

#ifdef GUI_WIN32

	void Painter::waitForFrame() {
		// Wait until we have rendered a frame, so that Windows think we did it to satisfy the paint
		// request.
		Nat old = atomicRead(repaintScreen);
		while (old == atomicRead(repaintScreen))
			os::UThread::leave();
	}

	bool Painter::doRepaintI() {
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
			target.target()->EndDraw();
			throw;
		}

		HRESULT r = target.target()->EndDraw();

		if (r == D2DERR_RECREATE_TARGET || r == DXGI_ERROR_DEVICE_RESET) {
			// Re-create our render target.
			destroy();
			create();
			// TODO: We probably want to re-draw ourselves here...
		}

		return more;
	}

	void Painter::present(Bool waitForVSync) {
		if (waitForVSync)
			target.swapChain()->Present(1, 0);
		else
			target.swapChain()->Present(0, 0);

		atomicWrite(repaintScreen, atomicRead(repaintScreen));
	}

	void Painter::invalidateWindow() {
		// We will probably never call this on Windows, but for completeness:
		InvalidateRect(attachedTo.hwnd(), NULL, FALSE);
	}


#endif
#ifdef GUI_GTK

	void Painter::waitForFrame() {
		// Just wait until we're not drawing at the moment.
		Lock::Guard z(lock);
	}

	bool Painter::ready() {
		// Wait until the previous frame is actually shown.
		return atomicRead(currentRepaint) == atomicRead(repaintCounter);
	}

	bool Painter::doRepaintI(bool waitForVSync, bool fromDraw) {
		if (!target.any())
			return continuous;

		bool more = false;
		Lock::Guard z(lock);

		CairoSurface *surface = target.surface();
		// If it wasn't created by now, something is *really* bad.
		if (!surface) {
			WARNING("Surface was not properly created!");
			return false;
		}
		cairo_surface_mark_dirty(surface->surface);

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

		cairo_surface_flush(surface->surface);

		// Tell Gtk+ we're ready to draw, unless a call to 'draw' is already queued.
		if (!fromDraw) {
#ifdef UI_MULTITHREAD
			app->repaint(attachedTo.widget());
#else
			if (GdkWindow *window = gtk_widget_get_window(attachedTo.widget()))
				gdk_window_invalidate_rect(window, NULL, true);
#endif
		}

		// TODO: Handle VSync?
		return more;
	}

	void Painter::beforeRepaint(RepaintParams *p) {
		// Required when we're rendering directly to a window surface.
		if (!target.any()) {
			if (attached()) {
				target = mgr->create(p);
				if (graphics)
					graphics->updateTarget(target);
			}
		}
	}

	void Painter::afterRepaint() {}

	void Painter::uiAfterRepaint(RepaintParams *params) {
		Lock::Guard z(lock);
		atomicWrite(repaintScreen, atomicRead(repaintScreen));

		if (CairoSurface *surface = target.surface()) {
			cairo_set_source_surface(params->ctx, surface->surface, 0, 0);
			cairo_paint(params->ctx);

			// Make sure Cairo does not use our surface outside the lock!
			cairo_surface_flush(cairo_get_group_target(params->ctx));

			// We're ready for the next frame now!
			if (continuous) {
				mgr->painterReady();
#ifdef UI_SINGLETHREAD
				invalidateWindow();
#endif
			}
		}
	}

	void Painter::invalidateWindow() {
		if (GdkWindow *window = gtk_widget_get_window(attachedTo.widget()))
			gdk_window_invalidate_rect(window, NULL, true);
	}

#endif
}
