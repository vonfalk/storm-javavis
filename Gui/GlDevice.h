#pragma once
#include "RenderInfo.h"
#include "Handle.h"

#ifdef GUI_GTK
namespace gui {

	// TODO: Remove 'GlContext.h' and rename these to GlContext.
	class GlDevice;

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

	private:
		// Current context.
		GlDevice *context;
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

		// Resize the surface. Alters 'cairo'.
		virtual void resize(Size s) = 0;
	};


	/**
	 * Encapsulation of either an EGL context or a GLX context.
	 */
	class GlDevice : NoCopy {
	public:
		// Destroy.
		virtual ~GlDevice();

		// Create a device for the current display.
		static GlDevice *create(Engine &e);

		// Cairo device for the current context.
		cairo_device_t *device;

		// Create a surface for a window using this context.
		virtual GlSurface *createSurface(GdkWindow *window, Size size) = 0;

	protected:
		// Create.
		GlDevice();

		// Create a cairo device for this context.
		virtual cairo_device_t *createDevice() = 0;
	};

	/**
	 * EGL context.
	 */
	class EglDevice : public GlDevice {
	public:
		// Destroy.
		~EglDevice();

		// Create an EGL device, returns null on failure.
		static EglDevice *create(Display *display);

		// Create a surface.
		virtual GlSurface *createSurface(GdkWindow *window, Size size);

	protected:
		// Create cairo device.
		virtual cairo_device_t *createDevice();

	private:
		// Create.
		EglDevice(EGLDisplay display);

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

			virtual void resize(Size s);

		private:
			EGLSurface surface;
		};
	};

	/**
	 * GLX context.
	 */
	class GlxDevice : public GlDevice {
	public:
		// Destroy.
		~GlxDevice();

		// Create a GLX device, returns null on failure.
		static GlxDevice *create(Display *display);

		// Create a surface.
		virtual GlSurface *createSurface(GdkWindow *window, Size size);

	protected:
		// Create cairo device.
		virtual cairo_device_t *createDevice();

	private:
		// Create.
		GlxDevice(Display *display);

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

			virtual void resize(Size s);

		private:
			::Window window;
		};
	};

}
#endif
