#include "stdafx.h"
#include "GlDevice.h"
#include "Graphics.h"
#include "RenderMgr.h"
#include "Exception.h"
#include "App.h"
#include "Window.h"
#include "Core/Convert.h"

#ifdef GUI_GTK

namespace gui {

	// Get the draw widget for a window:
	static GtkWidget *drawWidget(Engine &e, Handle handle) {
		Window *w = app(e)->findWindow(handle);
		if (!w)
			return handle.widget();
		else
			return w->drawWidget();
	}

	static void reportError(Engine &e, const char *error) {
		StrBuf *msg = new (e) StrBuf();
		*msg << S("Initialization of OpenGL failed: ") << toWChar(e, error)->v << S("\n");
		*msg << S("Try setting the environment variable ") << S(RENDER_ENV_NAME) << S(" to \"gtk\" or \"software\".");

		throw new (e) GuiError(msg->toS());
	}

	static void reportError(Engine &e, GError *error) {
		StrBuf *msg = new (e) StrBuf();
		*msg << S("Initialization of OpenGL failed: ") << toWChar(e, error->message)->v << S("\n");
		*msg << S("Try setting the environment variable ") << S(RENDER_ENV_NAME) << S(" to \"gtk\" or \"software\".");
		g_clear_error(&error);

		throw new (e) GuiError(msg->toS());
	}

	CairoGlDevice::CairoGlDevice(Engine &e) : e(e), context(null), device(null), surfaces(0), id(0) {
		TODO(L"It seems like we can use a single GL context.");
		// According to this e-mail, we can use a single GL context for all windows in the application:
		// https://mail.gnome.org/archives/gtk-list/2015-October/msg00045.html
		// If that works (also for Skia), we could probably simplify resouce management quite a bit.
	}

	CairoGlDevice::~CairoGlDevice() {
		destroy();
	}

	Surface *CairoGlDevice::createSurface(Handle window) {
		GdkWindow *win = gtk_widget_get_window(drawWidget(e, window));
		if (!win)
			return null;

		Size size(gtk_widget_get_allocated_width(window.widget()),
				gtk_widget_get_allocated_height(window.widget()));

		if (!context)
			create(win);

		if (id == 0)
			id = renderMgr(e)->allocId();

		surfaces++;
		return new CairoGlSurface(id, size, *this);
	}

	void CairoGlDevice::unrefContext() {
		if (surfaces == 0)
			return;

		if (--surfaces == 0)
			destroy();
	}

	void CairoGlDevice::create(GdkWindow *window) {
		GError *error = NULL;
		context = gdk_window_create_gl_context(window, &error);
		if (error) {
			g_object_unref(context);
			reportError(e, error);
		}

		// Use OpenGL ES, v2.0 or later.
		gdk_gl_context_set_use_es(context, true);
		gdk_gl_context_set_required_version(context, 2, 0);

		gdk_gl_context_realize(context, &error);
		if (error) {
			g_object_unref(context);
			reportError(e, error);
		}

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
			g_object_unref(context);
			context = null;
			reportError(e, "Failed to find the created GL context.");
		}

		gdk_gl_context_clear_current();
	}

	void CairoGlDevice::destroy() {
		if (device) {
			cairo_device_destroy(device);
			device = null;
		}

		if (context) {
			gdk_gl_context_clear_current();
			g_object_unref(context);
			context = null;
		}
	}

	CairoGlSurface::CairoGlSurface(Nat id, Size size, CairoGlDevice &owner)
		: CairoSurface(id, size, null), owner(owner), texture(0) {

		int width = size.w;
		int height = size.h;

		gdk_gl_context_clear_current();
		gdk_gl_context_make_current(owner.context);

		glGenTextures(1, &texture);

		glBindTexture(GL_TEXTURE_2D, texture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		// If we don't use ES, we need GL_BGRA instead of GL_RGBA
		// glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);

		surface = cairo_gl_surface_create_for_texture(owner.device, CAIRO_CONTENT_COLOR, texture, width, height);
		device = cairo_create(surface);
	}

	CairoGlSurface::~CairoGlSurface() {
		cairo_destroy(device);
		device = null;
		cairo_surface_destroy(surface);
		surface = null;

		// This check is important if objects are destroyed in the wrong order (slightly, at least).
		if (owner.context) {
			gdk_gl_context_make_current(owner.context);
			glDeleteTextures(1, &texture);
		}

		owner.unrefContext();
	}

	WindowGraphics *CairoGlSurface::createGraphics(Engine &e) {
		return new (e) CairoGraphics(*this, id, true);
	}

	void CairoGlSurface::resize(Size size, Float scale) {
		this->scale = scale;
		this->size = size;

		cairo_destroy(device);

		int width = size.w;
		int height = size.h;

		gdk_gl_context_make_current(owner.context);
		glBindTexture(GL_TEXTURE_2D, texture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

		cairo_surface_destroy(surface);
		surface = cairo_gl_surface_create_for_texture(owner.device, CAIRO_CONTENT_COLOR, texture, width, height);
		device = cairo_create(surface);
	}

	Surface::PresentStatus CairoGlSurface::present(bool waitForVSync) {
		return pRepaint;
	}

	void CairoGlSurface::repaint(RepaintParams *params) {
		// Documentation says we need to make this current, but it does not actually need that...
		gdk_gl_context_make_current(owner.context);
		gdk_cairo_draw_from_gl(params->cairo, params->window,
							texture, GL_TEXTURE, 1 /* scale */,
							0, 0, int(size.w), int(size.h));
		// Make sure everything is flushed now. Otherwise, we might see flickering when we start re-painting again.
		glFlush();
	}


}

#endif
