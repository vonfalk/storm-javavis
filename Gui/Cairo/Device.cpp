#include "stdafx.h"
#include "Device.h"
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

	CairoDevice::CairoDevice(Engine &e) : e(e), myId(0) {}

	Nat CairoDevice::id() {
		if (myId == 0)
			myId = renderMgr(e)->allocId();
		return myId;
	}

	CairoSwDevice::CairoSwDevice(Engine &e) : CairoDevice(e) {}

	Surface *CairoSwDevice::createSurface(Handle window) {
		Size size(gtk_widget_get_allocated_width(window.widget()),
				gtk_widget_get_allocated_height(window.widget()));
		cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_RGB24, size.w, size.h);

		return new CairoBlitSurface(id(), size, surface);
	}


	CairoGtkDevice::CairoGtkDevice(Engine &e) : CairoDevice(e) {}

	Surface *CairoGtkDevice::createSurface(Handle window) {
		GdkWindow *win = gtk_widget_get_window(drawWidget(e, window));
		if (!win)
			return null;

		Size size(gtk_widget_get_allocated_width(window.widget()),
				gtk_widget_get_allocated_height(window.widget()));

		return new CairoBlitSurface(id(), size,
									gdk_window_create_similar_surface(win, CAIRO_CONTENT_COLOR, int(size.w), int(size.h)));
	}


	CairoSurface::CairoSurface(Nat id, Size size)
		: Surface(size, 1.0f), surface(null), device(null), id(id) {}


	CairoSurface::CairoSurface(Nat id, Size size, cairo_surface_t *surface)
		: Surface(size, 1.0f), surface(surface), device(null), id(id) {

		if (surface)
			device = cairo_create(surface);
	}

	WindowGraphics *CairoSurface::createGraphics(Engine &e) {
		return new (e) CairoGraphics(*this, id, false);
	}

	void CairoSurface::resize(Size size, Float scale) {
		this->scale = scale;
		this->size = size;

		cairo_destroy(device);
		cairo_surface_t *tmp = cairo_surface_create_similar(surface, CAIRO_CONTENT_COLOR, int(size.w), int(size.h));
		cairo_surface_destroy(surface);
		surface = tmp;
		device = cairo_create(surface);
	}


	CairoBlitSurface::CairoBlitSurface(Nat id, Size size, cairo_surface_t *surface)
		: CairoSurface(id, size, surface) {}

	Surface::PresentStatus CairoBlitSurface::present(bool waitForVSync) {
		// We cannot do much here...
		return pRepaint;
	}

	void CairoBlitSurface::repaint(RepaintParams *params) {
		cairo_set_source_surface(params->cairo, surface, 0, 0);
		cairo_paint(params->cairo);

		// Make sure the operation is not pending, this function is called inside a lock.
		cairo_surface_flush(cairo_get_group_target(params->cairo));
	}


	CairoGlDevice::CairoGlDevice(Engine &e) : e(e) {
		TODO(L"It seems like we can use a single GL context.");
		// According to this e-mail, we can use a single GL context for all windows in the application:
		// https://mail.gnome.org/archives/gtk-list/2015-October/msg00045.html
		// If that works (also for Skia), we could probably simplify resouce management quite a bit.
	}

	Surface *CairoGlDevice::createSurface(Handle window) {
		GdkWindow *win = gtk_widget_get_window(drawWidget(e, window));
		if (!win)
			return null;

		Size size(gtk_widget_get_allocated_width(window.widget()),
				gtk_widget_get_allocated_height(window.widget()));

		Nat id = renderMgr(e)->allocId();
		return new CairoGlSurface(id, size, win);
	}

	static void reportError(const char *error) {
		Engine &e = runtime::someEngine();

		StrBuf *msg = new (e) StrBuf();
		*msg << S("Initialization of OpenGL failed: ") << toWChar(e, error)->v << S("\n");
		*msg << S("Try setting the environment variable ") << S(RENDER_ENV_NAME) << S(" to \"gtk\" or \"software\".");

		throw new (e) GuiError(msg->toS());
	}

	static void reportError(GError *error) {
		Engine &e = runtime::someEngine();

		StrBuf *msg = new (e) StrBuf();
		PVAR(error->message);
		*msg << S("Initialization of OpenGL failed: ") << toWChar(e, error->message)->v << S("\n");
		*msg << S("Try setting the environment variable ") << S(RENDER_ENV_NAME) << S(" to \"gtk\" or \"software\".");
		g_clear_error(&error);

		throw new (e) GuiError(msg->toS());
	}

	CairoGlSurface::CairoGlSurface(Nat id, Size size, GdkWindow *window)
		: CairoSurface(id, size, null), context(null), texture(0), dev(0) {

		GError *error = NULL;
		context = gdk_window_create_gl_context(window, &error);
		if (error) {
			g_object_unref(context);
			reportError(error);
		}

		// Use OpenGL ES, v2.0 or later.
		gdk_gl_context_set_use_es(context, true);
		gdk_gl_context_set_required_version(context, 2, 0);

		gdk_gl_context_realize(context, &error);
		if (error) {
			g_object_unref(context);
			reportError(error);
		}

		int width = size.w;
		int height = size.h;

		gdk_gl_context_make_current(context);

		glGenTextures(1, &texture);

		glBindTexture(GL_TEXTURE_2D, texture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		// If we don't use ES, we need GL_BGRA instead of GL_RGBA
		// glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);

		// Try to get the current context and create a Cairo device for it.
		GdkDisplay *gdkDisplay = gdk_window_get_display(window);
		if (GDK_IS_WAYLAND_DISPLAY(gdkDisplay)) {
			// Try EGL.
			GdkWaylandDisplay *display = GDK_WAYLAND_DISPLAY(gdkDisplay);
			struct wl_display *wlDisplay = gdk_wayland_display_get_wl_display(display);
			if (EGLContext eglContext = eglGetCurrentContext()) {
				dev = cairo_egl_device_create(eglGetDisplay((NativeDisplayType)wlDisplay), eglContext);
			}
		} else if (GDK_IS_X11_DISPLAY(gdkDisplay)) {
			// Try GLX first, and then EGL. Note: It seems like Gtk+ does not attempt to use EGL on X11 at the moment.
			Display *display = GDK_DISPLAY_XDISPLAY(gdkDisplay);
			if (GLXContext glxContext = glXGetCurrentContext()) {
				dev = cairo_glx_device_create(display, glxContext);
			} else if (EGLContext eglContext = eglGetCurrentContext()) {
				dev = cairo_egl_device_create(eglGetDisplay(display), eglContext);
			}
		}

		if (!dev) {
			g_object_unref(context);
			reportError("Failed to find the created GL context.");
		}

		surface = cairo_gl_surface_create_for_texture(dev, CAIRO_CONTENT_COLOR, texture, width, height);
		device = cairo_create(surface);
	}

	CairoGlSurface::~CairoGlSurface() {
		cairo_destroy(device);
		device = null;
		cairo_surface_destroy(surface);
		surface = null;

		gdk_gl_context_make_current(context);
		glDeleteTextures(1, &texture);

		cairo_device_destroy(dev);

		gdk_gl_context_clear_current();
		g_object_unref(context);
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

		gdk_gl_context_make_current(context);
		glBindTexture(GL_TEXTURE_2D, texture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

		cairo_surface_destroy(surface);
		surface = cairo_gl_surface_create_for_texture(dev, CAIRO_CONTENT_COLOR, texture, width, height);
		device = cairo_create(surface);
	}

	Surface::PresentStatus CairoGlSurface::present(bool waitForVSync) {
		return pRepaint;
	}

	void CairoGlSurface::repaint(RepaintParams *params) {
		gdk_gl_context_make_current(context);
		gdk_cairo_draw_from_gl(params->cairo, params->window,
							texture, GL_TEXTURE, 1 /* scale */,
							0, 0, int(size.w), int(size.h));
	}


}

#endif
