#include "stdafx.h"
#include "Painter.h"
#include "RenderResource.h"
#include "App.h"

namespace gui {

	Painter::Painter() : continuous(false), rendering(false), repaintCounter(0), currentRepaint(0) {
		attachedTo = Window::invalid;
		bgColor = app(engine())->defaultBgColor;
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

	void Painter::waitForFrame() {
		// Wait until we have rendered a frame, so that Windows think we did it to satisfy the paint
		// request.
		Nat old = repaintCounter;
		while (old == repaintCounter)
			os::UThread::leave();
	}

#ifdef GUI_WIN32

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

#endif
#ifdef GUI_GTK

	bool Painter::doRepaintI(bool waitForVSync) {
		if (!target.any())
			return continuous;

		bool more = false;

		// Clear the surface with the desired color.
		GlContext *ctx = target.context();
		ctx->activate();
		glClearColor(bgColor.r, bgColor.g, bgColor.b, bgColor.a);
		glClear(GL_COLOR_BUFFER_BIT);

		// Start a frame and set default parameters in case someone failed to clean up.
		nvgBeginFrame(ctx->nvg, target.size.w, target.size.h, 1.0f);
		nvgGlobalCompositeOperation(ctx->nvg, NVG_SOURCE_OVER);
		nvgLineCap(ctx->nvg, NVG_ROUND);
		// Note: we could set some more...

		try {
			graphics->beforeRender();
			more = render(graphics->size(), graphics);
			graphics->afterRender();
		} catch (...) {
			graphics->afterRender();
			nvgCancelFrame(ctx->nvg);
			throw;
		}

		nvgEndFrame(ctx->nvg);

		// TODO: handle VSync somehow?
		target.context()->swapBuffers();

		return more;
	}

	void Painter::beforeRepaint(RepaintParams *p) {
		if (!target.any()) {
			// Update the size, since it might have changed since we last observed it.
			Size sz = Size(gtk_widget_get_allocated_width(p->widget),
						gtk_widget_get_allocated_height(p->widget));
			target.size = sz;

			// Create the target.
			target.context(GlContext::create(p->target));
			if (graphics)
				graphics->updateTarget(target);
		}
	}

	void Painter::afterRepaint(RepaintParams *p) {
		// Nothing special needs to be done.
	}

#endif
}
