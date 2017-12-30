#pragma once
#include "RenderInfo.h"
#include "Handle.h"

#ifdef GUI_GTK
namespace gui {

	class GlContext;

	/**
	 * Implements the logic of creating and accessing a device in OpenGL on Linux.
	 */
	class Device : NoCopy {
	public:
		// Create.
		Device(Engine &e);

		// Destroy.
		~Device();

		// Attach a painter.
		RenderInfo attach(Handle window);

		// Resize the target of a painter.
		void resize(RenderInfo &info, Size size);

		// Create the context for a window (stage 2 of creation).
		RenderInfo create(GtkWidget *widget, GdkWindow *window);

		// Get a pango context for text rendering.
		inline PangoContext *pango() const { return pangoContext; }

	private:
		// Current context.
		GlContext *context;

		// Dummy context for text rendering.
		cairo_surface_t *pangoSurface;
		cairo_t *pangoTarget;
		PangoContext *pangoContext;
	};


	/**
	 * Represents a surface we can draw to in OpenGL.
	 */
	class GlSurface : NoCopy {
	public:
		// Create.
		GlSurface(cairo_surface_t *surface);

		// Destroy.
		virtual ~GlSurface();

		// Current cairo surface.
		cairo_surface_t *cairo;

		// Swap buffers.
		void swapBuffers();

		// Resize the surface.
		void resize(Size s);
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

		// Create a surface for a window using this context.
		virtual GlSurface *createSurface(GdkWindow *window, Size size) = 0;

	protected:
		// Create.
		GlContext();

		// Create a cairo device for this context.
		virtual cairo_device_t *createDevice() = 0;
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
