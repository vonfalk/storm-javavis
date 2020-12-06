#include "stdafx.h"
#include "Painter.h"
#include "RenderResource.h"
#include "App.h"
#include "Surface.h"

namespace gui {

	Painter::Painter()
		: continuous(false), synchronizedPresent(false),
		  repaintCounter(0), currentRepaint(0), surface(null) {

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
	}

	void Painter::doRepaint(bool waitForVSync, bool fromDraw) {
		if (!surface)
			return;

		bool more = false;
		try {
			Lock::Guard z(lock);
			repaintCounter++;
			more = doRepaintI(waitForVSync, fromDraw);
		} catch (...) {
			throw;
		}

		if (more != continuous) {
			continuous = more;
			if (more) {
				// Register!
				mgr->painterReady();
			}
		}
	}

	bool Painter::doRepaintI(bool waitForVSync, bool fromDraw) {
		if (!surface)
			return false;

		// Make sure we're not presenting while drawing.
		Lock::Guard z(lock);

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

		// Don't try to present if something failed.
		if (!ok)
			return more;

		// Try to present the frame.
		switch (surface->present(waitForVSync)) {
		case Surface::pSuccess:
			// All is well. Remember that we presented the frame.
			synchronizedPresent = false;
			currentRepaint = repaintCounter;
			break;
		case Surface::pRecreate:
			// We need to re-create the render target.
			destroy();
			surface = mgr->attach(this, attachedTo);
			graphics = surface->createGraphics(engine());
			// TODO: We probably want to redraw immediately here. We can at least schedule a re-draw
			// on the next frame! On Windows, this is almost equivalent, as we won't wait for vsync
			// since Present() failed. On Linux, this will not happen.
			return true;
		case Surface::pRepaint:
			// If this was not a direct call from "repaint", we need to schedule a repaint through
			// the windowing system.
			synchronizedPresent = true;
			if (!fromDraw)
				repaintAttachedWindow();
			break;
		case Surface::pFailure:
			// Something else failed. Not too much we can do, I guess. This likely indicates either
			// a bug in the implementation, or an out of memory condition.
			WARNING(L"Rendering failure! Either you are out of (graphics) memory, or you have found a bug.");
			break;
		}

		return more;
	}

	void Painter::uiAfterRepaint(RepaintParams *params) {
		if (synchronizedPresent) {
			Lock::Guard z(lock);

			// If we don't have any surface, then we're being destroyed.
			if (!surface)
				return;

			surface->repaint(params);
			currentRepaint = repaintCounter;

			// We're ready for the next frame now!
			if (continuous) {
				mgr->painterReady();
#ifdef UI_SINGLETHREAD
				repaintAttachedWindow();
#endif
			}
		}
	}

	bool Painter::ready() {
		// Wait until the previous frame is actually shown.
		return atomicRead(currentRepaint) == atomicRead(repaintCounter);
	}

	void Painter::waitForFrame() {
		if (synchronizedPresent) {
			// Just wait until we're not drawing at the moment.
			Lock::Guard z(lock);
		} else {
			// Wait until we have rendered a frame, so that Windows think we did it to satisfy the
			// paint request.
			Nat old = currentRepaint;
			while (old == currentRepaint)
				os::UThread::leave();
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
		if (!ready()) {
			// There is a frame ready right now! No need to wait even if we're not in continuous mode!
		} else if (continuous) {
			waitForFrame();
		} else {
			doRepaint(false, true);
		}
	}

	void Painter::uiAttach(Window *to) {
		// Note: We're blocking the UI thread here, as Gtk+ requires some intricate interaction with
		// the UI thread to create GL context, etc.
		os::Future<void, Semaphore> result;
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

	void Painter::repaintAttachedWindow() {
		WARNING(L"This operation is not supported on Win32, as it should not be needed.");
	}

#endif
#ifdef GUI_GTK

	void Painter::repaintAttachedWindow() {
#ifdef UI_MULTITHREAD
		app->repaint(attachedTo.widget());
#else
		gtk_widget_queue_draw(attachedTo.widget());
#endif
	}

#endif
}
