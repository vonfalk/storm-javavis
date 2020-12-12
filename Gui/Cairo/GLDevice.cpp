#include "stdafx.h"
#include "GLDevice.h"
#include "Graphics.h"
#include "RenderMgr.h"
#include "Exception.h"
#include "App.h"
#include "Window.h"
#include "TextMgr.h"

#ifdef GUI_GTK

namespace gui {

	CairoGLContext::CairoGLContext(GLDevice *owner, GdkWindow *window, GdkGLContext *context)
		: Context(owner, window, context), device(null) {

		gdk_gl_context_make_current(context);

		// Try to get the current context and create a Cairo device for it.
		GdkDisplay *gdkDisplay = gdk_window_get_display(window);
		if (GDK_IS_WAYLAND_DISPLAY(gdkDisplay)) {
			// Try EGL.
			GdkWaylandDisplay *display = GDK_WAYLAND_DISPLAY(gdkDisplay);
			struct wl_display *wlDisplay = gdk_wayland_display_get_wl_display(display);
			if (EGLContext eglContext = eglGetCurrentContext()) {
				device = cairo_egl_device_create(eglGetDisplay((NativeDisplayType)wlDisplay), eglContext);
			}
		} else if (GDK_IS_X11_DISPLAY(gdkDisplay)) {
			// Try GLX first, and then EGL. Note: It seems like Gtk+ does not attempt to use EGL on X11 at the moment.
			Display *display = GDK_DISPLAY_XDISPLAY(gdkDisplay);
			if (GLXContext glxContext = glXGetCurrentContext()) {
				device = cairo_glx_device_create(display, glxContext);
			} else if (EGLContext eglContext = eglGetCurrentContext()) {
				device = cairo_egl_device_create(eglGetDisplay(display), eglContext);
			}
		}

		if (!device) {
			StrBuf *msg = new (owner->e) StrBuf();
			*msg << S("Initialization of OpenGL failed: Failed to extract the current GL context.\n");
			*msg << S("Try setting the environment variable ") << S(ENV_RENDER_BACKEND) << S(" to \"gtk\" or \"software\".");
			throw new (owner->e) GuiError(msg->toS());
		}

		gdk_gl_context_clear_current();
	}

	CairoGLContext::~CairoGLContext() {
		gdk_gl_context_clear_current();
		gdk_gl_context_make_current(context);
		cairo_device_destroy(device);
	}

	CairoGLDevice::CairoGLDevice(Engine &e) : GLDevice(e) {}

	CairoGLContext *CairoGLDevice::createContext(GdkWindow *window, GdkGLContext *context) {
		return new CairoGLContext(this, window, context);
	}

	Surface *CairoGLDevice::createSurface(GtkWidget *widget, Context *context) {
		Size size(gtk_widget_get_allocated_width(widget),
				gtk_widget_get_allocated_height(widget));

		if (context->id == 0) {
			context->id = renderMgr(e)->allocId();
		}

		return new CairoGLSurface(size, static_cast<CairoGLContext *>(context));
	}

	TextMgr *CairoGLDevice::createTextMgr() {
		return new CairoText();
	}

	CairoGLSurface::CairoGLSurface(Size size, CairoGLContext *context)
		: CairoSurface(context->id, size, null), context(context), texture(0) {

		int width = size.w;
		int height = size.h;

		gdk_gl_context_clear_current();
		gdk_gl_context_make_current(context->context);

		glGenTextures(1, &texture);

		glBindTexture(GL_TEXTURE_2D, texture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		// If we don't use ES, we need GL_BGRA instead of GL_RGBA
		// glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);

		surface = cairo_gl_surface_create_for_texture(context->device, CAIRO_CONTENT_COLOR, texture, width, height);
		device = cairo_create(surface);
	}

	CairoGLSurface::~CairoGLSurface() {
		cairo_destroy(device);
		device = null;
		cairo_surface_destroy(surface);
		surface = null;

		gdk_gl_context_clear_current();
		gdk_gl_context_make_current(context->context);
		glDeleteTextures(1, &texture);

		context->unref();
	}

	WindowGraphics *CairoGLSurface::createGraphics(Engine &e) {
		return new (e) CairoGraphics(*this, id, true);
	}

	void CairoGLSurface::resize(Size size, Float scale) {
		this->scale = scale;
		this->size = size;

		cairo_destroy(device);

		int width = size.w;
		int height = size.h;

		// The clear is since Cairo will switch GL context from time to time, turning make_context
		// into a noop some times, when it really should not be a noop.
		gdk_gl_context_clear_current();
		gdk_gl_context_make_current(context->context);

		glBindTexture(GL_TEXTURE_2D, texture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

		cairo_surface_destroy(surface);
		surface = cairo_gl_surface_create_for_texture(context->device, CAIRO_CONTENT_COLOR, texture, width, height);
		device = cairo_create(surface);
	}

	Surface::PresentStatus CairoGLSurface::present(bool waitForVSync) {
		return pRepaint;
	}

	void CairoGLSurface::repaint(RepaintParams *params) {
		gdk_gl_context_make_current(context->context);
		gdk_cairo_draw_from_gl(params->cairo, params->window,
							texture, GL_TEXTURE, 1 /* scale */,
							0, 0, int(size.w), int(size.h));
		// Make sure everything is flushed now. Otherwise, we might see flickering when we start re-painting again.
		glFlush();
	}


}

#endif
