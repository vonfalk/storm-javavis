#include "stdafx.h"
#include "GlContext.h"
#include "Exception.h"

#ifdef GUI_GTK

namespace gui {

	GlContext *GlContext::create(GdkWindow *window) {
		GlContext *result = EglContext::create(window);
		if (!result)
			result = GlxContext::create(window);
		if (!result)
			throw GuiError(L"Failed to initialize OpenGL.");

		result->activate();
		result->nvg = nvgCreateGL2(NVG_ANTIALIAS | NVG_STENCIL_STROKES);

		return result;
	}

	GlContext::GlContext() : nvg(null) {}

	GlContext::~GlContext() {
		if (nvg)
			nvgDeleteGL2(nvg);

		if (active == this)
			active = null;
	}

	GlContext *GlContext::active = null;

	void GlContext::activate() {
		if (active == this)
			return;
		setActive();
	}


	/**
	 * EGL
	 */

	EglContext *EglContext::create(GdkWindow *window) {
		// TODO: Detect Wayland as well.
		EGLNativeWindowType eWindow = GDK_WINDOW_XID(window);
		Display *xDisplay = GDK_DISPLAY_XDISPLAY(gdk_window_get_display(window));
		EGLDisplay eDisplay = eglGetDisplay(xDisplay);

		EGLBoolean ok = eglInitialize(eDisplay, NULL, NULL);

		EglContext *result = null;
		if (ok) {
			result = new EglContext(eDisplay);
			ok = eglBindAPI(EGL_OPENGL_API);
		}

		if (ok)
			ok = result->display->initialize();

		if (ok) {
			result->window = eWindow;
			result->surface = eglCreateWindowSurface(eDisplay, result->display->config, eWindow, NULL);
			if (!result->surface)
				ok = false;
		}

		if (!ok) {
			delete result;
			result = null;
		}

		return result;
	}

	std::map<EGLDisplay, EglContext::DisplayData *> EglContext::displays;

	EglContext::DisplayData::DisplayData(EGLDisplay display) : display(display), context(null), config(null), refs(1) {}

	EglContext::DisplayData::~DisplayData() {
		if (context)
			eglDestroyContext(display, context);

		// TODO: We might not want to terminate the display as Gtk+ might use it.
		if (display)
			eglTerminate(display);
	}

	bool EglContext::DisplayData::initialize() {
		if (context)
			return true;

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

		context = eglCreateContext(display, config, EGL_NO_CONTEXT, NULL);
		if (!context)
			return false;

		return true;
	}

	EglContext::EglContext(EGLDisplay display) : surface(null) {
		DisplayMap::iterator i = displays.find(display);
		if (i == displays.end()) {
			this->display = new DisplayData(display);
			displays.insert(make_pair(display, this->display));
		} else {
			this->display = i->second;
			this->display->addRef();
		}
	}

	EglContext::~EglContext() {
		if (surface)
			eglDestroySurface(display->context, surface);

		if (display)
			display->release();
	}

	void EglContext::setActive() {
		eglMakeCurrent(display->display, surface, surface, display->context);
	}

	void EglContext::swapBuffers() {
		eglSwapBuffers(display->display, surface);
	}


	/**
	 * GLX
	 */

	GlxContext *GlxContext::create(GdkWindow *window) {
		::Window xWindow = GDK_WINDOW_XID(window);
		Display *xDisplay = GDK_DISPLAY_XDISPLAY(gdk_window_get_display(window));

		// We do not bother to check the version. If it works, it works... fbConfig requires 1.3 or
		// later, but on some X-servers it works anyway.
		// glXQueryVersion(xDisplay, &major, &minor);

		GlxContext *result = new GlxContext(xDisplay, xWindow);
		bool ok = result->display->initialize();

		if (!ok) {
			delete result;
			result = null;
		}
		return result;
	}

	std::map<Display *, GlxContext::DisplayData *> GlxContext::displays;

	GlxContext::DisplayData::DisplayData(Display *display) : display(display), context(null), refs(1) {}

	GlxContext::DisplayData::~DisplayData() {
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

	bool GlxContext::DisplayData::initialize() {
		if (context)
			return true;

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

	GlxContext::GlxContext(Display *display, ::Window window) : window(window) {
		DisplayMap::iterator i = displays.find(display);
		if (i == displays.end()) {
			this->display = new DisplayData(display);
			displays.insert(make_pair(display, this->display));
		} else {
			this->display = i->second;
			this->display->addRef();
		}
	}

	GlxContext::~GlxContext() {
		if (display)
			display->release();
	}

	void GlxContext::setActive() {
		glXMakeCurrent(display->display, window, display->context);
	}

	void GlxContext::swapBuffers() {
		glXSwapBuffers(display->display, window);
	}

}

#endif
