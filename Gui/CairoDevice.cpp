#include "stdafx.h"
#include "CairoDevice.h"
#include "Exception.h"
#include "App.h"
#include "Painter.h"

#ifdef GUI_GTK
namespace gui {

	static GlDevice *createGl(Engine &e) {
		// Should be safe to call 'app' here since it is created by now.
		App *app = gui::app(e);

		GlDevice *result = null;
		GdkDisplay *gdkDisplay = app->defaultDisplay();
		if (GDK_IS_WAYLAND_DISPLAY(gdkDisplay)) {
			// EGL is our only option here.
			GdkWaylandDisplay *display = GDK_WAYLAND_DISPLAY(gdkDisplay);
			result = EglDevice::create(e, gdk_wayland_display_get_wl_display(display));
		} else if (GDK_IS_X11_DISPLAY(gdkDisplay)) {
			Display *display = GDK_DISPLAY_XDISPLAY(gdkDisplay);

			// GLX seems faster in practice. It is also more stable using X11 forwarding. Might be
			// required for Wayland support, though. EGL works fine when running locally, so it is a
			// good fallback.
			result = GlxDevice::create(e, display);
			if (!result)
				result = EglDevice::create(e, display);
		} else {
			throw new (e) GuiError(S("You are using an unsupported windowing system. Wayland and X11 are supported for OpenGL."));
		}

		if (!result)
			throw new (e) GuiError(S("Failed to initialize OpenGL."));

		if (cairo_device_status(result->device))
			throw new (e) GuiError(S("Failed to initialize Cairo. Cairo requires at least OpenGL 2.0 or OpenGL ES 2.0."));

		cairo_gl_device_set_thread_aware(result->device, TRUE);

		return result;
	}

	static CairoDevice *create(Engine &e, const char *preference) {
		if (strcmp(preference, "gtk") == 0) {
			return new GtkDevice();
		} else if (strcmp(preference, "sw") == 0) {
			return new SoftwareDevice();
		} else if (strcmp(preference, "software") == 0) {
			return new SoftwareDevice();
		} else if (strcmp(preference, "gl") == 0) {
			return createGl(e);
		} else {
			throw new (e) GuiError(S("The supplied value of STORM_RENDER_BACKEND is not supported."));
		}
	}

	static CairoDevice *create(Engine &e) {
		const char *preference = getenv("STORM_RENDER_BACKEND");
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

	DeviceType Device::type() {
		return device->type();
	}

	RenderInfo Device::attach(Handle window) {
		RenderInfo info;

		info.size = Size(gtk_widget_get_allocated_width(window.widget()),
						gtk_widget_get_allocated_height(window.widget()));

		// Create surface as appropriate.
		info.surface(device->createSurface(info.size));
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

	RenderInfo Device::create(RepaintParams *params) {
		RenderInfo info;
		info.size = Size(gtk_widget_get_allocated_width(params->widget),
						gtk_widget_get_allocated_height(params->widget));

		// Create the surface as appropriate.
		info.surface(device->createSurface(info.size, params));
		info.target(info.surface()->target);

		return info;
	}


	/**
	 * Generic surface.
	 */

	CairoSurface::CairoSurface(cairo_surface_t *surface) : surface(surface) {
		target = cairo_create(surface);
	}

	CairoSurface::~CairoSurface() {
		cairo_destroy(target);
		cairo_surface_destroy(surface);
	}

	void CairoSurface::resize(Size sz) {
		cairo_destroy(target);
		cairo_surface_t *tmp = cairo_surface_create_similar(surface, CAIRO_CONTENT_COLOR, int(sz.w), int(sz.h));
		cairo_surface_destroy(surface);
		surface = tmp;
		target = cairo_create(surface);
	}


	/**
	 * Generic device.
	 */

	CairoSurface *CairoDevice::createPangoSurface(Size size) {
		return createSurface(size);
	}


	/**
	 * Software device.
	 */

	CairoSurface *SoftwareDevice::createSurface(Size size) {
		return new CairoSurface(cairo_image_surface_create(CAIRO_FORMAT_RGB24, size.w, size.h));
	}

	CairoSurface *SoftwareDevice::createSurface(Size size, RepaintParams *) {
		return createSurface(size);
	}


	/**
	 * Gtk device.
	 */

	CairoSurface *GtkDevice::createSurface(Size size) {
		return null;
	}

	CairoSurface *GtkDevice::createSurface(Size size, RepaintParams *params) {
		cairo_surface_t *target = cairo_get_target(params->ctx);
		return new CairoSurface(cairo_surface_create_similar(target, CAIRO_CONTENT_COLOR, int(size.w), int(size.h)));
	}

	CairoSurface *GtkDevice::createPangoSurface(Size size) {
		return new CairoSurface(cairo_image_surface_create(CAIRO_FORMAT_RGB24, size.w, size.h));
	}


	/**
	 * GL device.
	 */

	GlDevice::GlDevice(cairo_device_t *device) : device(device) {}

	GlDevice::~GlDevice() {
		if (device)
			cairo_device_destroy(device);
	}

	CairoSurface *GlDevice::createSurface(Size size) {
		return new GlSurface(this, size);
	}

	CairoSurface *GlDevice::createSurface(Size size, RepaintParams *) {
		return createSurface(size);
	}


	/**
	 * GL surface.
	 */

	GlSurface::GlSurface(GlDevice *device, Size size)
		: CairoSurface(cairo_gl_surface_create(device->device, CAIRO_CONTENT_COLOR, int(size.w), int(size.h))),
		  device(device) {}

	void GlSurface::resize(Size size) {
		cairo_destroy(target);
		cairo_surface_destroy(surface);
		surface = cairo_gl_surface_create(device->device, CAIRO_CONTENT_COLOR, int(size.w), int(size.h));
		target = cairo_create(surface);
	}


	/**
	 * GLX device.
	 */

	GlxDevice::GlxDevice(Engine &e, Display *display)
		: GlDevice(null), display(display), context(null) {}

	GlxDevice::~GlxDevice() {
		if (device)
			cairo_device_destroy(device);
		device = null;

		if (context)
			glXDestroyContext(display, context);
	}

	GlxDevice *GlxDevice::create(Engine &e, Display *display) {
		GlxDevice *result = new GlxDevice(e, display);
		if (!result->init()) {
			delete result;
			return null;
		} else {
			result->device = cairo_glx_device_create(result->display, result->context);
			return result;
		}
	}

	// Find the best configuration.
	static GLXFBConfig pickConfig(Display *display, int *attrs) {
		int configCount = 0;
		GLXFBConfig *configs = glXChooseFBConfig(display, DefaultScreen(display), attrs, &configCount);
		if (!configs)
			return null;
		if (configCount == 0) {
			XFree(configs);
			return null;
		}

		// Just take the first one. It is probably good enough for now.
		// We might want to look at glXGetFBConfigAttrib(display, configs[i], ?, out);
		// for ? = GLX_SAMPLE_BUFFERS and ? = GLX_SAMPLES to pick a config with good anti-aliasing.
		GLXFBConfig result = configs[0];
		XFree(configs);

		return result;
	}

	bool GlxDevice::init() {
		int attrs[] = {
			GLX_X_RENDERABLE, True,
			GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
			GLX_RENDER_TYPE, GLX_RGBA_BIT,
			GLX_X_VISUAL_TYPE, GLX_TRUE_COLOR,
			GLX_RED_SIZE, 8,
			GLX_GREEN_SIZE, 8,
			GLX_BLUE_SIZE, 8,
			GLX_STENCIL_SIZE, 8,
			GLX_DOUBLEBUFFER, True,
			None
		};
		// Use FBConfig if it is available. We might need to enforce this later, since we might
		// require the ability to create pbuffers.
		if (GLXFBConfig config = pickConfig(display, attrs)) {
			context = glXCreateNewContext(display, config, GLX_RGBA_TYPE, NULL, True);
			if (context)
				return true;
		}

		// Fallback on the slightly older glXChooseVisual.
		int cvAttrs[] = {
			GLX_RGBA,
			GLX_RED_SIZE, 8,
			GLX_GREEN_SIZE, 8,
			GLX_BLUE_SIZE, 8,
			GLX_STENCIL_SIZE, 8,
			GLX_DOUBLEBUFFER, True,
			None
		};
		if (XVisualInfo *info = glXChooseVisual(display, DefaultScreen(display), cvAttrs)) {
			context = glXCreateContext(display, info, NULL, True);
			XFree(info);

			if (context)
				return true;
		}
		return false;
	}


	/**
	 * EGL device.
	 */

	EglDevice::EglDevice(Engine &e, EGLDisplay display)
		: GlDevice(null), display(display), context(null), config(null) {}

	EglDevice::~EglDevice() {
		if (device)
			cairo_device_destroy(device);
		device = null;

		if (context)
			eglDestroyContext(display, context);
		if (display)
			eglTerminate(display);
	}

	EglDevice *EglDevice::create(Engine &e, Display *xDisplay) {
		EGLDisplay display = eglGetDisplay(xDisplay);
		if (!eglInitialize(display, NULL, NULL))
			return null;

		EglDevice *me = new EglDevice(e, display);
		if (!me->init()) {
			delete me;
			return null;
		} else {
			return me;
		}
	}

	EglDevice *EglDevice::create(Engine &e, struct wl_display *wlDisplay) {
		EGLDisplay display = eglGetDisplay((NativeDisplayType)wlDisplay);
		if (!eglInitialize(display, NULL, NULL))
			return null;

		EglDevice *me = new EglDevice(e, display);
		if (!me->init()) {
			delete me;
			return null;
		} else {
			return me;
		}
	}

	bool EglDevice::init() {
		EGLint attributes[] = {
			EGL_RED_SIZE, 8,
			EGL_GREEN_SIZE, 8,
			EGL_BLUE_SIZE, 8,
			EGL_STENCIL_SIZE, 8,
			EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
			EGL_NONE,
		};
		EGLint configCount = 0;
		if (!eglChooseConfig(display, attributes, &config, 1, &configCount))
			return false;
		if (configCount == 0)
			return false;

		// Make sure we get 'regular' OpenGL, not OpenGL ES, since that does not seem to work
		// properly with Cairo, at least not over X11 forwarding.
		eglBindAPI(EGL_OPENGL_API);

		EGLint contextAttrs[] = {
			// Specify OpenGL ES 2. (ignored when we're using OpenGL)
			EGL_CONTEXT_CLIENT_VERSION, 2,
			EGL_NONE,
		};
		context = eglCreateContext(display, config, EGL_NO_CONTEXT, contextAttrs);
		if (!context)
			return false;

		device = cairo_egl_device_create(display, context);
		return true;
	}

}
#endif
