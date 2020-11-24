#include "stdafx.h"
#include "Painter.h"
#include "RenderResource.h"
#include "App.h"
#include "Surface.h"

namespace gui {

	Painter::Painter() : continuous(false), repaintCounter(0), currentRepaint(0), surface(null) {
		attachedTo = Window::invalid;
		app = gui::app(engine());
		bgColor = app->defaultBgColor;
		mgr = gui::renderMgr(engine());
		lock = new (this) Lock();
	}

	Painter::~Painter() {
		detach();
	}

	Bool Painter::render(Size size, Graphics *graphics) {
		return false;
	}

	void Painter::attach(Window *to) {
		Handle handle = to->handle();
		if (handle != attachedTo) {
			detach();

			surface = mgr->attach(this, handle);
			if (surface) {
				graphics = surface->createGraphics(engine());
				attachedTo = handle;
			} else {
				// Will be attached again later. This currently only happens in Gtk+.
				attachedTo = Window::invalid;
			}
		}
	}

	void Painter::detach() {
		if (attachedTo != Window::invalid) {
			attachedTo = Window::invalid;
			destroy();
		}
	}

	void Painter::resize(Size sz, Float scale) {
		if (surface) {
			// Do not attempt to resize the drawing surface if we're currently drawing to it.
			Lock::Guard z(lock);

			// This seems to not be neccessary. Probably because the resources are actually
			// associated to the underlying D3D device, and not the RenderTarget.
			// If the resize call fails for some reason, this is probably the cause.

			// destroyResources();

			surface->resize(sz, scale);
			if (graphics)
				graphics->surfaceResized();
		}
	}

	void Painter::destroy() {
		// Wait until we're not rendering anymore.
		Lock::Guard z(lock);

		if (graphics) {
			graphics->surfaceDestroyed();
			graphics->destroy();
		}
		graphics = null;
		delete surface;
		surface = null;

		mgr->detach(this);
	}

	void Painter::repaint() {
		if (!surface)
			return;

		if (continuous) {
			waitForFrame();
		} else {
			doRepaint(false, false);
		}

		afterRepaint();
	}

	void Painter::doRepaint(bool waitForVSync, bool fromDraw) {
		if (!surface)
			return;

		bool more = false;
		try {
			Lock::Guard z(lock);
			more = doRepaintI(waitForVSync, fromDraw);
		} catch (...) {
			repaintCounter++;
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

#ifdef UI_SINGLETHREAD

	void Painter::repaintI(RepaintParams *params) {
		beforeRepaint(params);

		// Do we need to render the frame now, or did we already render something before?
		if (ready())
			doRepaint(false, true);

		afterRepaint();
	}

	void Painter::uiAttach(Window *to) {
		attach(to);
	}

	void Painter::uiDetach() {
		detach();
	}

	void Painter::uiResize(Size size, Float scale) {
		resize(size, scale);
	}

	void Painter::uiRepaint(RepaintParams *par) {
		repaintI(par);
		uiAfterRepaint(par);
	}

#endif
#ifdef UI_MULTITHREAD

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

	void Painter::uiResize(Size size, Float scale) {
		os::Future<void> result;
		Painter *me = this;
		os::FnCall<void, 3> params = os::fnCall().add(me).add(size).add(scale);
		os::UThread::spawn(address(&Painter::resize), true, params, result, &thread->thread());
		result.result();
	}

	void Painter::uiRepaint(RepaintParams *par) {
		os::Future<void> result;
		Painter *me = this;
		os::FnCall<void, 2> params = os::fnCall().add(me).add(par);
		os::UThread::spawn(address(&Painter::repaintI), true, params, result, &thread->thread());
		result.result();

		uiAfterRepaint(par);
	}

#endif
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

	bool Painter::doRepaintI(bool waitForVSync, bool fromDraw) {
		if (!surface)
			return false;

		bool more = false;
		bool ok = false;

		try {
			graphics->beforeRender(bgColor);
			more = render(surface->size / surface->scale, graphics);
			ok = graphics->afterRender();
		} catch (...) {
			graphics->afterRender();
			throw;
		}

		if (ok) {
			ok = surface->present(waitForVSync);
		}

		if (!ok) {
			// Re-create our render target.
			destroy();

			surface = mgr->attach(this, attachedTo);
			graphics = surface->createGraphics(engine());
			// TODO: We probably want to re-draw ourselves here...
		}

		return more;
	}

	void Painter::beforeRepaint(RepaintParams *handle) {}

	void Painter::afterRepaint() {
		currentRepaint = repaintCounter;
	}

	void Painter::uiAfterRepaint(RepaintParams *handle) {}

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
		if (!surface)
			return continuous;

		bool more = false;
		bool ok = false;
		Lock::Guard z(lock);

		// If it wasn't created by now, something is *really* bad.
		if (!surface) {
			WARNING("Surface was not properly created!");
			return false;
		}

		try {
			graphics->beforeRender(bgColor);
			more = render(surface->size / surface->scale, graphics);
			ok = graphics->afterRender();
		} catch (...) {
			graphics->afterRender();
			throw;
		}

		// TODO: Handle OK being false?
		(void)ok;

		// Tell Gtk+ we're ready to draw, unless a call to 'draw' is already queued.
		if (!fromDraw) {
#ifdef UI_MULTITHREAD
			app->repaint(attachedTo.widget());
#else
			gtk_widget_queue_draw(attachedTo.widget());
#endif
		}

		// TODO: Handle VSync?
		return more;
	}

	void Painter::beforeRepaint(RepaintParams *p) {
		// Required when we're rendering directly to a window surface.

		// TODO: Is this needed anymore?
		// if (!target.any()) {
		// 	if (attached()) {
		// 		if (graphics)
		// 			graphics->updateTarget(target);
		// 	}
		// }
	}

	void Painter::afterRepaint() {}

	void Painter::uiAfterRepaint(RepaintParams *params) {
		Lock::Guard z(lock);
		currentRepaint = repaintCounter;

		if (CairoSurface *surface = target.surface()) {
			surface->blit(params->ctx);

			// We're ready for the next frame now!
			if (continuous) {
				mgr->painterReady();
#ifdef UI_SINGLETHREAD
				gtk_widget_queue_draw(attachedTo.widget());
#endif
			}
		}
	}

#endif
}
