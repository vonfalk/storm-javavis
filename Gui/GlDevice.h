#pragma once
#include "RenderInfo.h"
#include "Handle.h"

#ifdef GUI_GTK
namespace gui {

	class GlContext;
	class RepaintParams;

	/**
	 * Implements the logic of creating and accessing a device in OpenGL on Linux.
	 */
	class Device : NoCopy {
	public:
		// Create.
		Device(Engine &e);

		// Destroy.
		~Device();

		// Attach a painter. If it returns a RenderInfo where 'any()' returns false, call 'create' later.
		RenderInfo attach(Handle window);

		// Resize the target of a painter.
		void resize(RenderInfo &info, Size size);

		// Create a painter the first time it is drawn.
		RenderInfo create(RepaintParams *params);

		// Get a pango context for text rendering.
		inline PangoContext *pango() const { return pangoContext; }

	private:
		// Current context.
		GlContext *context;

		// Dummy context for text rendering.
		GlSurface *pangoSurface;
		cairo_t *pangoTarget;
		PangoContext *pangoContext;
	};


	/**
	 * Represents a surface we can draw to in OpenGL.
	 */
	class GlSurface : NoCopy {
	public:
		// Create. If 'offscreen' is true, we can not use cairo_gl_surface_set_size.
		GlSurface(cairo_surface_t *surface);

		// Destroy.
		virtual ~GlSurface();

		// Current cairo surface.
		cairo_surface_t *cairo;

		// Attach to a specific window (if required).
		virtual void attach(GdkWindow *to);

		// Swap buffers.
		virtual void swapBuffers();

		// Resize the surface. Might alter 'cairo'.
		virtual void resize(Size s);
	};


	/**
	 * Represents an offscreen surface.
	 */
	class GlOffscreenSurface : public GlSurface {
	public:
		// Create.
		GlOffscreenSurface(GlContext *owner, Size size);

		// Swap buffers (no-op here).
		virtual void swapBuffers();

		// Resize.
		virtual void resize(Size s);

	protected:
		// Owner.
		GlContext *owner;
	};


	/**
	 * Represents an offscreen surface where 'swapBuffers' copies the contents to the screen.
	 */
	class GlDoubleSurface : public GlOffscreenSurface {
	public:
		// Create.
		GlDoubleSurface(GlContext *owner, Size size);

		// Destroy.
		~GlDoubleSurface();

		// Attach to a window.
		virtual void attach(GdkWindow *to);

		// Swap buffers.
		virtual void swapBuffers();

		// Resize.
		virtual void resize(Size s);

	private:
		// Surface for the onscreen buffer.
		GlSurface *screenSurface;

		// Cairo surface for the onscreen buffer.
		cairo_t *screen;
	};


	/**
	 * Encapsulation of either an EGL context or a GLX context.
	 */
	class GlContext : NoCopy {
	public:
		// Destroy.
		virtual ~GlContext();

		// Create a device for the current display.
		static GlContext *create(Engine &e);

		// Cairo device for the current context.
		cairo_device_t *device;

		// Create an off-screen surface.
		virtual GlSurface *createSurface(Size size);

		// Create an off-screen double surface.
		virtual GlSurface *createDoubleSurface(Size size);

		// Create a surface for a window using this context.
		virtual GlSurface *createSurface(GdkWindow *window, Size size) = 0;

	protected:
		// Create.
		GlContext();

		// Create a cairo device for this context.
		virtual cairo_device_t *createDevice() = 0;

		// Destroy the current device. Called by subclasses to destroy the cairo_device before any
		// GL context is destroyed.
		void destroyDevice();
	};

	/**
	 * Surface for software rendering. Not really a GlContext...
	 */
	class GlSoftwareContext : public GlContext {
	public:
		// Create.
		GlSoftwareContext();

		// Create an off-screen surface.
		virtual GlSurface *createSurface(Size size);

		// Create an off-screen double surface.
		virtual GlSurface *createDoubleSurface(Size size);

		// Create a surface for a window.
		virtual GlSurface *createSurface(GdkWindow *window, Size size);

	protected:
		// Create device.
		virtual cairo_device_t *createDevice();

	private:
		class SwSurface : public GlSurface {
		public:
			// Create.
			SwSurface(Size size);

			// Swap buffers (no-op here).
			virtual void swapBuffers();

			// Resize.
			virtual void resize(Size s);
		};
	};

	/**
	 * EGL context.
	 */
	class EglContext : public GlContext {
	public:
		// Destroy.
		~EglContext();

		// Create an EGL device, returns null on failure.
		static EglContext *create(Display *display);

		// Create a surface.
		virtual GlSurface *createSurface(GdkWindow *window, Size size);

	protected:
		// Create cairo device.
		virtual cairo_device_t *createDevice();

	private:
		// Create.
		EglContext(EGLDisplay display);

		// Initialize.
		bool initialize();

		// The display.
		EGLDisplay display;

		// The context.
		EGLContext context;

		// The configuration used for this context.
		EGLConfig config;

		// Surfaces from EGL.
		class Surface : public GlSurface {
		public:
			Surface(cairo_surface_t *cairo, EGLSurface surface);
			~Surface();

		private:
			EGLSurface surface;
		};
	};

	/**
	 * GLX context.
	 */
	class GlxContext : public GlContext {
	public:
		// Destroy.
		~GlxContext();

		// Create a GLX device, returns null on failure.
		static GlxContext *create(Display *display);

		// Create a surface.
		virtual GlSurface *createSurface(GdkWindow *window, Size size);

	protected:
		// Create cairo device.
		virtual cairo_device_t *createDevice();

	private:
		// Create.
		GlxContext(Display *display);

		// Initialize.
		bool initialize();

		// The display.
		Display *display;

		// The context.
		GLXContext context;

		// Surfaces from GLX.
		class Surface : public GlSurface {
		public:
			Surface(cairo_surface_t *surface, ::Window window);

		private:
			::Window window;
		};
	};

}
#endif
