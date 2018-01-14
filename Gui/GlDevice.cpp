#include "stdafx.h"
#include "GlDevice.h"
#include "Exception.h"
#include "App.h"
#include "Painter.h"

#ifdef GUI_GTK
namespace gui {

	Device::Device(Engine &e) {
		context = GlContext::create(e);

		pangoSurface = context->createSurface(Size(1, 1));
		pangoTarget = cairo_create(pangoSurface->cairo);
		pangoContext = pango_cairo_create_context(pangoTarget);
	}

	Device::~Device() {
		g_object_unref(pangoContext);
		cairo_destroy(pangoTarget);
		delete pangoSurface;

		delete context;
	}

	RenderInfo Device::attach(Handle window) {
		RenderInfo info;

		info.size = Size(gtk_widget_get_allocated_width(window.widget()),
						gtk_widget_get_allocated_height(window.widget()));

#if GTK_RENDER_IS_CAIRO(GTK_MODE)
		info.surface(context->createSurface(info.size));
#elif GTK_RENDER_IS_COPY(GTK_MODE)
		info.surface(context->createDoubleSurface(info.size));
#else
		// In case of GTK_RENDER_SYNC or GTK_RENDER_SWAP, create the surface later.
#endif

		if (info.surface())
			info.target(cairo_create(info.surface()->cairo));

		return info;
	}

	void Device::resize(RenderInfo &info, Size size) {
		info.size = size;

		cairo_t *oldTarget = info.target();
		cairo_surface_t *oldSurface = info.surface()->cairo;

		info.surface()->resize(size);

		if (oldSurface != info.surface()->cairo) {
			cairo_destroy(oldTarget);
			info.target(cairo_create(info.surface()->cairo));
		}
	}

	// Create a surface that renders to an actual window.
	RenderInfo Device::create(RepaintParams *params) {
		RenderInfo info;
		info.size = Size(gtk_widget_get_allocated_width(params->widget),
						gtk_widget_get_allocated_height(params->widget));
		info.surface(context->createSurface(params->target, info.size));
		info.target(cairo_create(info.surface()->cairo));
		return info;
	}


	/**
	 * GL surface.
	 */

	GlSurface::GlSurface(cairo_surface_t *cairo) : cairo(cairo) {}

	GlSurface::~GlSurface() {
		if (cairo)
			cairo_surface_destroy(cairo);
	}

	void GlSurface::attach(GdkWindow *to) {}

	void GlSurface::swapBuffers() {
		cairo_gl_surface_swapbuffers(cairo);
	}

	void GlSurface::resize(Size s) {
		cairo_gl_surface_set_size(cairo, s.w, s.h);
	}


	/**
	 * Offscreen surface.
	 */

	GlOffscreenSurface::GlOffscreenSurface(GlContext *owner, Size s) :
		GlSurface(cairo_gl_surface_create(owner->device, CAIRO_CONTENT_COLOR, s.w, s.h)),
		owner(owner) {}

	void GlOffscreenSurface::swapBuffers() {}

	void GlOffscreenSurface::resize(Size s) {
		cairo_surface_destroy(cairo);
		cairo = cairo_gl_surface_create(owner->device, CAIRO_CONTENT_COLOR, s.w, s.h);
	}


	/**
	 * Double surface.
	 */

	GlDoubleSurface::GlDoubleSurface(GlContext *owner, Size s) :
		GlOffscreenSurface(owner, s),
		screenSurface(null),
		screen(null) {}

	GlDoubleSurface::~GlDoubleSurface() {
		if (screen)
			cairo_destroy(screen);
		delete screenSurface;
	}

	void GlDoubleSurface::attach(GdkWindow *to) {
		if (screen)
			return;

		Size size(gdk_window_get_width(to), gdk_window_get_height(to));
		screenSurface = owner->createSurface(to, size);
		screen = cairo_create(screenSurface->cairo);
	}

	void GlDoubleSurface::swapBuffers() {
		cairo_surface_flush(cairo);

		cairo_set_source_surface(screen, cairo, 0, 0);
		cairo_paint(screen);
		screenSurface->swapBuffers();
	}

	void GlDoubleSurface::resize(Size s) {
		GlOffscreenSurface::resize(s);
		if (screenSurface)
			screenSurface->resize(s);
	}

	/**
	 * GL backends.
	 */

	GlContext::GlContext() : device(null) {}

	GlContext::~GlContext() {
		destroyDevice();
	}

	GlContext *GlContext::create(Engine &e) {
		// Should be safe to call 'app' here since it is created by now.
		App *app = gui::app(e);
		// Note: We should support Wayland as well.
		Display *display = GDK_DISPLAY_XDISPLAY(app->defaultDisplay());

#if GTK_RENDER_IS_SW(GTK_MODE)
		return new GlSoftwareContext();
#endif

		// It seems GLX is faster in practice. It is also more stable using X11 forwarding. Might be
		// required for Wayland support, though. EGL works fine when running locally, so it is a
		// good fallback.
		GlContext *result = GlxContext::create(display);
		if (!result)
			result = EglContext::create(display);
		if (!result)
			throw GuiError(L"Failed to initialize OpenGL");

		result->device = result->createDevice();
		if (result->device) {
			if (cairo_device_status(result->device) != CAIRO_STATUS_SUCCESS)
				throw GuiError(L"Failed to initialize Cario. Cairo requires at least OpenGL 2.0 or OpenGL ES 2.0.");

			cairo_gl_device_set_thread_aware(result->device, TRUE);
		}

		return result;
	}

	void GlContext::destroyDevice() {
		if (device)
			cairo_device_destroy(device);
		device = null;
	}

	GlSurface *GlContext::createSurface(Size size) {
		return new GlOffscreenSurface(this, size);
	}

	GlSurface *GlContext::createDoubleSurface(Size size) {
		return new GlDoubleSurface(this, size);
	}


	/**
	 * Software.
	 */

	GlSoftwareContext::GlSoftwareContext() {}

	GlSurface *GlSoftwareContext::createSurface(Size size) {
		return new SwSurface(size);
	}

	GlSurface *GlSoftwareContext::createDoubleSurface(Size size) {
		assert(false, L"Not supported!");
		return null;
	}

	GlSurface *GlSoftwareContext::createSurface(GdkWindow *window, Size size) {
		assert(false, L"Not supported!");
		return null;
	}

	cairo_device_t *GlSoftwareContext::createDevice() {
		return null;
	}

	GlSoftwareContext::SwSurface::SwSurface(Size size) :
		GlSurface(cairo_image_surface_create(CAIRO_FORMAT_ARGB32, size.w, size.h)) {}

	void GlSoftwareContext::SwSurface::swapBuffers() {}

	void GlSoftwareContext::SwSurface::resize(Size s) {
		cairo_surface_destroy(cairo);
		cairo = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, s.w, s.h);
	}


	/**
	 * EGL
	 */

	EglContext *EglContext::create(Display *xDisplay) {
		EGLDisplay display = eglGetDisplay(xDisplay);
		if (!eglInitialize(display, NULL, NULL))
			return null;

		EglContext *me = new EglContext(display);
		if (!me->initialize()) {
			delete me;
			return null;
		} else {
			return me;
		}
	}

	EglContext::EglContext(EGLDisplay display) : display(display), context(null), config(null) {}

	EglContext::~EglContext() {
		destroyDevice();

		if (context)
			eglDestroyContext(display, context);

		if (display)
			eglTerminate(display);
	}

	bool EglContext::initialize() {
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

		return true;
	}

	cairo_device_t *EglContext::createDevice() {
		return cairo_egl_device_create(display, context);
	}

	GlSurface *EglContext::createSurface(GdkWindow *gWindow, Size size) {
		EGLNativeWindowType window = GDK_WINDOW_XID(gWindow);

		EGLint attrs[] = {
			EGL_RENDER_BUFFER, EGL_BACK_BUFFER,
			EGL_NONE,
		};
		EGLSurface surface = eglCreateWindowSurface(display, config, window, attrs);
		if (!surface)
			throw GuiError(L"Failed to create an EGL surface for a window.");

		cairo_surface_t *cairo = cairo_gl_surface_create_for_egl(device, surface, size.w, size.h);
		if (cairo_surface_status(cairo) != CAIRO_STATUS_SUCCESS)
			throw GuiError(L"Failed to create a cairo surface for a window.");

		return new Surface(cairo, surface);
	}

	EglContext::Surface::Surface(cairo_surface_t *cairo, EGLSurface surface) :
		GlSurface(cairo), surface(surface) {}

	EglContext::Surface::~Surface() {
		EGLDisplay display = cairo_egl_device_get_display(cairo_surface_get_device(cairo));
		eglDestroySurface(display, surface);
	}


	/**
	 * GLX
	 */

	GlxContext *GlxContext::create(Display *display) {
		GlxContext *result = new GlxContext(display);
		if (!result->initialize()) {
			delete result;
			return null;
		} else {
			return result;
		}
	}

	GlxContext::GlxContext(Display *display) : display(display), context(null) {}

	GlxContext::~GlxContext() {
		destroyDevice();

		if (context)
			glXDestroyContext(display, context);
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

	bool GlxContext::initialize() {
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

	cairo_device_t *GlxContext::createDevice() {
		return cairo_glx_device_create(display, context);
	}

	GlSurface *GlxContext::createSurface(GdkWindow *gWindow, Size size) {
		::Window window = GDK_WINDOW_XID(gWindow);
		cairo_surface_t *cairo = cairo_gl_surface_create_for_window(device, window, size.w, size.h);
		if (cairo_surface_status(cairo) != CAIRO_STATUS_SUCCESS)
			throw GuiError(L"Failed to create a cairo surface for a window.");

		return new Surface(cairo, window);
	}

	GlxContext::Surface::Surface(cairo_surface_t *cairo, ::Window window) :
		GlSurface(cairo), window(window) {}

}
#endif
