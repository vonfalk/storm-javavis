#include "stdafx.h"
#include "Device.h"
#include "App.h"
#include "RenderMgr.h"
#include "Window.h"
#include "Exception.h"
#include "Graphics.h"
#include "PangoMgr.h"
#include "SkParagraphMgr.h"
#include "Core/Convert.h"

#ifdef GUI_GTK

namespace gui {

	SkiaContext::SkiaContext(GLDevice *owner, GdkWindow *window, GdkGLContext *context)
		: Context(owner, window, context) {

		gdk_gl_context_clear_current();
		gdk_gl_context_make_current(context);

		// Try to get the current context and create an Interface for it.
		sk_sp<const GrGLInterface> interface;
		GdkDisplay *gdkDisplay = gdk_window_get_display(window);
		if (GDK_IS_WAYLAND_DISPLAY(gdkDisplay)) {
			// Try EGL.
			if (eglGetCurrentContext()) {
				interface = GrGLMakeAssembledGLESInterface(null, &egl_get);
			}
		} else if (GDK_IS_X11_DISPLAY(gdkDisplay)) {
			// Try GLX first, and then EGL. Note: It seems like Gtk does not attempt to use EGL on X11 at the moment.
			if (glXGetCurrentContext()) {
				interface = GrGLMakeAssembledGLESInterface(null, &glx_get);
			} else {
				interface = GrGLMakeAssembledGLESInterface(null, &egl_get);
			}
		}

		if (!interface)
			throw new (runtime::someEngine()) GuiError(S("Failed to initialize OpenGL with Skia."));

		skia = GrDirectContext::MakeGL(interface);
		if (!skia)
			throw new (runtime::someEngine()) GuiError(S("Failed to initialize OpenGL with Skia."));
	}

	SkiaContext::~SkiaContext() {}


	SkiaDevice::SkiaDevice(Engine &e) : GLDevice(e) {}

	SkiaContext *SkiaDevice::createContext(GdkWindow *window, GdkGLContext *context) {
		return new SkiaContext(this, window, context);
	}

	Surface *SkiaDevice::createSurface(GtkWidget *widget, Context *context) {
		Size size(gtk_widget_get_allocated_width(widget),
				gtk_widget_get_allocated_height(widget));

		return new SkiaSurface(size, static_cast<SkiaContext *>(context));
	}

	TextMgr *SkiaDevice::createTextMgr() {
		// TODO: Do we want to allow switching between Pango and SkParagraph if both are available?
#if HAS_SK_PARAGRAPH
		return new SkParagraphMgr();
#else
		return new SkiaPangoMgr();
#endif
	}


	// Needs to match the setup below.
	static const int stencilBits = 8;
	// This does not always seem to work when set to something else than 1. Skia can do
	// anti-aliasing anyway, so it is fine.
	static const int msaa = 1;

	SkiaSurface::SkiaSurface(Size size, SkiaContext *context)
		: Surface(size, 1.0f), context(context) {

		gdk_gl_context_make_current(context->context);

		glGenFramebuffers(1, &framebuffer);
		glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

		glGenRenderbuffers(1, &colorbuffer);
		glGenRenderbuffers(1, &stencilbuffer);

		glBindRenderbuffer(GL_RENDERBUFFER, colorbuffer);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_RGB8, size.w, size.h);

		// TODO: We might be able to use an entire 32 bits per pixel for the stencil? It seems like
		// Skia does not use the depth part of it anyway. Or perhaps just 8bpp for the stencil buffer?
		glBindRenderbuffer(GL_RENDERBUFFER, stencilbuffer);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, size.w, size.h);

		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0_EXT, GL_RENDERBUFFER, colorbuffer);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER, stencilbuffer);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT_EXT, GL_RENDERBUFFER, stencilbuffer);

		glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);


		GrGLFramebufferInfo info;
		info.fFBOID = framebuffer;
		info.fFormat = GL_RGB8;
		target = GrBackendRenderTarget(size.w, size.h, msaa, stencilBits, info);
		surface = SkSurface::MakeFromBackendRenderTarget(context->skia.get(), target,
														kBottomLeft_GrSurfaceOrigin,
														kRGB_888x_SkColorType,
														nullptr, nullptr);

		canvas = surface->getCanvas();
	}

	SkiaSurface::~SkiaSurface() {
		gdk_gl_context_clear_current();
		gdk_gl_context_make_current(context->context);
		glDeleteRenderbuffers(1, &colorbuffer);
		glDeleteRenderbuffers(1, &stencilbuffer);
		glDeleteFramebuffers(1, &framebuffer);

		context->unref();
	}

	void SkiaSurface::resize(Size size, Float scale) {
		this->size = size;
		this->scale = scale;

		gdk_gl_context_clear_current();
		gdk_gl_context_make_current(context->context);
		glBindRenderbuffer(GL_RENDERBUFFER, colorbuffer);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_RGB8, size.w, size.h);

		glBindRenderbuffer(GL_RENDERBUFFER, stencilbuffer);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, size.w, size.h);

		// Clear out the old surface.
		surface = sk_sp<SkSurface>();

		// Create the surface again.
		GrGLFramebufferInfo info;
		info.fFBOID = framebuffer;
		info.fFormat = GL_RGB8;
		target = GrBackendRenderTarget(size.w, size.h, msaa, stencilBits, info);
		surface = SkSurface::MakeFromBackendRenderTarget(context->skia.get(), target,
														kBottomLeft_GrSurfaceOrigin,
														kRGB_888x_SkColorType,
														nullptr, nullptr);

		canvas = surface->getCanvas();
	}

	Surface::PresentStatus SkiaSurface::present(bool waitForVSync) {
		return pRepaint;
	}

	void SkiaSurface::repaint(RepaintParams *params) {
		gdk_gl_context_make_current(context->context);
		gdk_cairo_draw_from_gl(params->cairo, params->window,
							colorbuffer, GL_RENDERBUFFER, 1 /* scale */,
							0, 0, int(size.w), int(size.h));
		// Flush GL state from the copy to GTK. Otherwise, we might see flickering.
		glFlush();
	}

	WindowGraphics *SkiaSurface::createGraphics(Engine &e) {
		return new (e) SkiaGraphics(*this, context->id);
	}

}

#endif
