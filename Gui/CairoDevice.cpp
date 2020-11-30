#include "stdafx.h"
#include "CairoDevice.h"
#include "Exception.h"
#include "App.h"
#include "Painter.h"
#include "Skia.h"

#ifdef GUI_GTK

namespace gui {

	// Name of the environment variable.
	#define ENV_NAME "STORM_RENDER_BACKEND"

	static CairoDevice *create(Engine &e, const char *preference) {
		if (strcmp(preference, "gtk") == 0) {
			return new GtkDevice();
		} else if (strcmp(preference, "sw") == 0) {
			return new SoftwareDevice();
		} else if (strcmp(preference, "software") == 0) {
			return new SoftwareDevice();
		} else if (strcmp(preference, "gl") == 0) {
			return new GlDevice();
		} else if (strcmp(preference, "skia") == 0) {
			return new SkiaDevice();
		} else {
			throw new (e) GuiError(S("The supplied value of STORM_RENDER_BACKEND is not supported."));
		}
	}

	static CairoDevice *create(Engine &e) {
		TODO(L"Think about a good standard for STORM_RENDER_BACKEND");
		// On X11 forwarding, Gtk seems to work better, but how do we detect that?
		return create(e, "gl");

		const char *preference = getenv(ENV_NAME);
		if (preference)
			return create(e, preference);
		else
			return create(e, "gtk");
	}

	Device::Device(Engine &e) {
		device = gui::create(e);

		pangoSurface = device->createPangoSurface(Size(1, 1));
		pangoContext = pango_cairo_create_context(pangoSurface->target);
	}

	Device::~Device() {
		g_object_unref(pangoContext);
		delete pangoSurface;
		delete device;
	}

	RenderInfo Device::attach(Handle window) {
		RenderInfo info;

		info.size = Size(gtk_widget_get_allocated_width(window.widget()),
						gtk_widget_get_allocated_height(window.widget()));

		// Create surface as appropriate.
		info.surface(device->createSurface(info.size, window));
		if (info.surface())
			info.target(info.surface()->target);

		return info;
	}

	void Device::resize(RenderInfo &info, Size size) {
		info.size = size;

		if (info.surface()) {
			info.surface()->resize(size);
			info.target(info.surface()->target);
		}
	}


	/**
	 * Generic surface.
	 */

	CairoSurface::CairoSurface(cairo_surface_t *surface) : surface(surface) {
		target = cairo_create(surface);
	}

	CairoSurface::~CairoSurface() {
		if (target)
			cairo_destroy(target);
		if (surface)
			cairo_surface_destroy(surface);
	}

	void CairoSurface::resize(Size sz) {
		cairo_destroy(target);
		cairo_surface_t *tmp = cairo_surface_create_similar(surface, CAIRO_CONTENT_COLOR, int(sz.w), int(sz.h));
		cairo_surface_destroy(surface);
		surface = tmp;
		target = cairo_create(surface);
	}

	void CairoSurface::blit(cairo_t *to) {
		cairo_set_source_surface(to, surface, 0, 0);
		cairo_paint(to);
	}

	/**
	 * Abstract device.
	 */

	CairoSurface *CairoDevice::createPangoSurface(Size size) {
		return new CairoSurface(cairo_image_surface_create(CAIRO_FORMAT_RGB24, size.w, size.h));
	}

	/**
	 * Software device.
	 */

	CairoSurface *SoftwareDevice::createSurface(Size size, Handle) {
		return new CairoSurface(cairo_image_surface_create(CAIRO_FORMAT_RGB24, size.w, size.h));
	}


	/**
	 * Gtk device.
	 */

	CairoSurface *GtkDevice::createSurface(Size size, Handle window) {
		GdkWindow *gWin = gtk_widget_get_window(window.widget());
		return new CairoSurface(gdk_window_create_similar_surface(gWin, CAIRO_CONTENT_COLOR, size.w, size.h));
	}

	/**
	 * GL surface.
	 */

	static void reportError(const char *error) {
		Engine &e = runtime::someEngine();

		StrBuf *msg = new (e) StrBuf();
		*msg << S("Initialization of OpenGL failed: ") << error << S("\n");
		*msg << S("Try setting the environment variable ") << ENV_NAME << S(" to \"gtk\" or \"software\".");

		throw new (e) GuiError(msg->toS());
	}

	static void reportError(GError *error) {
		Engine &e = runtime::someEngine();

		StrBuf *msg = new (e) StrBuf();
		*msg << S("Initialization of OpenGL failed: ") << error->message << S("\n");
		*msg << S("Try setting the environment variable ") << ENV_NAME << S(" to \"gtk\" or \"software\".");
		g_clear_error(&error);

		throw new (e) GuiError(msg->toS());
	}

	GlSurface::GlSurface(GdkWindow *window, Size size) : CairoSurface(), device(null), width(size.w), height(size.h) {
		GError *error = NULL;
		context = gdk_window_create_gl_context(window, &error);
		if (error) {
			g_object_unref(&context);
			reportError(error);
		}

		// Use OpenGL ES, v2.0 or later.
		gdk_gl_context_set_use_es(context, true);
		gdk_gl_context_set_required_version(context, 2, 0);

		gdk_gl_context_realize(context, &error);
		if (error) {
			g_object_unref(&context);
			reportError(error);
		}

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
			g_object_unref(&context);
			reportError("Failed to find the created GL context.");
		}

		surface = cairo_gl_surface_create_for_texture(device, CAIRO_CONTENT_COLOR, texture, width, height);
		target = cairo_create(surface);
	}

	GlSurface::~GlSurface() {
		cairo_destroy(target);
		target = null;
		cairo_surface_destroy(surface);
		surface = null;

		gdk_gl_context_make_current(context);
		glDeleteTextures(1, &texture);

		cairo_device_destroy(device);

		gdk_gl_context_clear_current();
		g_object_unref(context);
	}

	void GlSurface::resize(Size size) {
		cairo_destroy(target);

		width = int(size.w);
		height = int(size.h);

		gdk_gl_context_make_current(context);
		glBindTexture(GL_TEXTURE_2D, texture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

		cairo_surface_destroy(surface);
		surface = cairo_gl_surface_create_for_texture(device, CAIRO_CONTENT_COLOR, texture, width, height);
		target = cairo_create(surface);
	}

	void GlSurface::blit(cairo_t *to) {
		cairo_surface_flush(surface);

		GdkWindow *window = gdk_gl_context_get_window(context);

		gdk_gl_context_make_current(context);
		gdk_cairo_draw_from_gl(to, window,
							texture, GL_TEXTURE, 1 /* scale */,
							0, 0, width, height);
	}

	/**
	 * GL device.
	 */

	CairoSurface *GlDevice::createSurface(Size size, Handle window) {
		GdkWindow *w = gtk_widget_get_window(window.widget());
		return new GlSurface(w, size);
	}

	SkiaSurface::SkiaSurface(GdkWindow *window, Size size) : CairoSurface(), width(size.w), height(size.h) {
		GError *error = NULL;
		context = gdk_window_create_gl_context(window, &error);
		if (error) {
			g_object_unref(&context);
			reportError(error);
		}

		// Use OpenGL ES, v2.0 or later.
		gdk_gl_context_set_use_es(context, true);
		gdk_gl_context_set_required_version(context, 2, 0);

		gdk_gl_context_realize(context, &error);
		if (error) {
			g_object_unref(&context);
			reportError(error);
		}

		gdk_gl_context_make_current(context);

		int stencilBits = 8; // Need to match the setup below.
		int msaa = 1; // This does not seem to always work when set to 4, for example. Not all
					  // backends seem to support this (maybe we need to do enable it)

		glGenFramebuffers(1, &framebuffer);
		glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

		glGenRenderbuffers(1, &renderbuffer);
		glGenRenderbuffers(1, &stencil);

		glBindRenderbuffer(GL_RENDERBUFFER, renderbuffer);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_RGB8, width, height);

		glBindRenderbuffer(GL_RENDERBUFFER, stencil);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);

		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0_EXT, GL_RENDERBUFFER, renderbuffer);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER, stencil); // Might skip
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT_EXT, GL_RENDERBUFFER, stencil);

		glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

		// Try to get the current context and create a Interface for it.
		sk_sp<const GrGLInterface> interface;
		GdkDisplay *gdkDisplay = gdk_window_get_display(window);
		if (GDK_IS_WAYLAND_DISPLAY(gdkDisplay)) {
			// Try EGL.
			if (eglGetCurrentContext()) {
				interface = GrGLMakeAssembledGLESInterface(null, &egl_get);
			}
		} else if (GDK_IS_X11_DISPLAY(gdkDisplay)) {
			// Try GLX first, and then EGL. Note: It seems like Gtk+ does not attempt to use EGL on X11 at the moment.
			if (glXGetCurrentContext()) {
				interface = GrGLMakeAssembledGLESInterface(null, &glx_get);
			} else if (eglGetCurrentContext()) {
				interface = GrGLMakeAssembledGLESInterface(null, &egl_get);
			}
		}

		if (!interface) {
			PLN(L"Failed to initialize GL!");
		}

		PVAR(interface.get());

		skiaContext = GrDirectContext::MakeGL(interface);
		PVAR(skiaContext.get());

		GrGLFramebufferInfo info;
		info.fFBOID = framebuffer;
		info.fFormat = GL_RGB8;
		// This must be alive as long as the surface is alive:
		skiaTarget = GrBackendRenderTarget(width, height, msaa, stencilBits, info);
		// SkSurfaceProps props; // last parameter
		skiaSurface = SkSurface::MakeFromBackendRenderTarget(skiaContext.get(), skiaTarget, kBottomLeft_GrSurfaceOrigin, kRGB_888x_SkColorType, nullptr, nullptr);

		PVAR(skiaSurface.get());
		SkCanvas *canvas = skiaSurface->getCanvas();
		PLN(L"Canvas: " << canvas);
		canvas->drawColor(SkColors::kRed);
		canvas->flush();
		PLN(L"Done!");

		surface = cairo_image_surface_create(CAIRO_FORMAT_RGB24, width, height);
		this->target = cairo_create(surface);

		i = 0;
	}

	SkiaSurface::~SkiaSurface() {
		cairo_destroy(target);
		target = null;
		cairo_surface_destroy(surface);
		surface = null;

		gdk_gl_context_make_current(context);
		glDeleteRenderbuffers(1, &renderbuffer);
		glDeleteRenderbuffers(1, &stencil);
		glDeleteFramebuffers(1, &framebuffer);

		gdk_gl_context_clear_current();
		g_object_unref(context);
	}

	void SkiaSurface::resize(Size size) {
		cairo_destroy(target);

		width = int(size.w);
		height = int(size.h);

		gdk_gl_context_make_current(context);
		glBindRenderbuffer(GL_RENDERBUFFER, renderbuffer);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_RGB8, width, height);

		glBindRenderbuffer(GL_RENDERBUFFER, stencil);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);

		TODO(L"Resize! Skia!");

		cairo_surface_destroy(surface);
		surface = cairo_image_surface_create(CAIRO_FORMAT_RGB24, width, height);
		target = cairo_create(surface);
	}

	void SkiaSurface::blit(cairo_t *to) {
		cairo_surface_flush(surface);

		GdkWindow *window = gdk_gl_context_get_window(context);

		gdk_gl_context_make_current(context);

		glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

		SkPaint line(SkColors::kGreen);

		SkCanvas *canvas = skiaSurface->getCanvas();
		canvas->resetMatrix();
		canvas->drawColor(SkColors::kRed);
		canvas->drawRect(SkRect{2.0f, 2.0f, 80.0f, 80.0f + i}, line);
		canvas->drawLine(0.0f, 0.0f, 150.0f, 150.0f - i, line);
		i++;
		canvas->flush();

		gdk_cairo_draw_from_gl(to, window,
							renderbuffer, GL_RENDERBUFFER, 1 /* scale */,
							0, 0, width, height);
	}

	SkiaSurface *SkiaDevice::createSurface(Size size, Handle window) {
		GdkWindow *w = gtk_widget_get_window(window.widget());
		return new SkiaSurface(w, size);
	}

}
#endif
