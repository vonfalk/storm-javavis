#pragma once
#include "RenderInfo.h"
#include "Handle.h"

#ifdef GUI_GTK
namespace gui {

	class CairoDevice;
	class RepaintParams;

	/**
	 * Implements the logic of managing a Cairo device on Linux, whatever backend has been selected
	 * (e.g. "native", "software" or OpenGL).
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

		// Get a Pango context for text rendering.
		inline PangoContext *pango() const { return pangoContext; }

	private:
		// Current device.
		CairoDevice *device;

		// Dummy context for text rendering.
		CairoSurface *pangoSurface;
		PangoContext *pangoContext;
	};

	/**
	 * A generic representation of a Cairo surface.
	 */
	class CairoSurface : NoCopy {
	public:
		// Create.
		CairoSurface(cairo_surface_t *surface);

		// Destroy.
		virtual ~CairoSurface();

		// Current surface.
		cairo_surface_t *surface;

		// Current target.
		cairo_t *target;

		// Resize the surface. Might re-create 'surface' and/or 'target'.
		virtual void resize(Size s);
	};


	/**
	 * A generic representation of a Cairo backend. This essentially describes how to create new Cairo surfaces.
	 */
	class CairoDevice : NoCopy {
	public:
		// Create a surface for when the window is first attached. Might return null if the surface
		// shall be created later.
		virtual CairoSurface *createSurface(Size size) = 0;

		// Create a surface for when the window is repainted. May not return null.
		virtual CairoSurface *createSurface(Size size, RepaintParams *params) = 0;

		// Create a surface for Pango font rendering. Like 'createSurface', but may not fail.
		// Default implementation calls 'createSurface', which may not always be suitable.
		virtual CairoSurface *createPangoSurface(Size size);
	};

	/**
	 * A fully software rendered Cairo backend.
	 */
	class SoftwareDevice : public CairoDevice {
	public:
		// This never fails in this implementation, as we can simply create the device at any time.
		virtual CairoSurface *createSurface(Size size);

		// Basically a call to 'createSurface', as it should never be called.
		virtual CairoSurface *createSurface(Size size, RepaintParams *params);
	};

	/**
	 * A backend that re-uses whatever backend Gtk+ is using. This means that we need to delay
	 * creation of surfaces until painting is actually done.
	 */
	class GtkDevice : public CairoDevice {
	public:
		// This will always fail, as we wait for the first draw call in order to be able to duplicate that device.
		virtual CairoSurface *createSurface(Size size);

		// Create the surface here!
		virtual CairoSurface *createSurface(Size size, RepaintParams *params);

		// As such, we will also need to override the creation of Pango surfaces.
		virtual CairoSurface *createPangoSurface(Size size);
	};


	/**
	 * A backend that uses OpenGL with Cairo-assisted flipping.
	 *
	 * This might not work smoothly with some graphic drivers (e.g. the new iris driver on mesa).
	 */
	class GlDevice : public CairoDevice {
	public:
		// Destroy.
		~GlDevice();

		// The OpenGL device.
		cairo_device_t *device;

		// Create surfaces. Will never fail.
		virtual CairoSurface *createSurface(Size size);

		// Basically a call to 'createSurface', as it should never be called.
		virtual CairoSurface *createSurface(Size size, RepaintParams *params);

	protected:
		// Create.
		GlDevice(cairo_device_t *device);
	};


	/**
	 * GL surface.
	 */

	class GlSurface : public CairoSurface {
	public:
		// Create with a specified size.
		GlSurface(GlDevice *device, Size size);

		// Custom resize.
		virtual void resize(Size s);

	private:
		// Owning device.
		GlDevice *device;
	};


	/**
	 * GLX device.
	 */
	class GlxDevice : public GlDevice {
	public:
		~GlxDevice();

		// Create a device. Might fail.
		static GlxDevice *create(Engine &e, Display *display);

	private:
		// Create.
		GlxDevice(Engine &e, Display *display);

		// The display.
		Display *display;

		// The context.
		GLXContext context;

		// Init.
		bool init();
	};


	/**
	 * EGL device.
	 */
	class EglDevice : public GlDevice {
	public:
		~EglDevice();

		// Create a device. Might fail.
		static EglDevice *create(Engine &e, Display *display);
		static EglDevice *create(Engine &e, struct wl_display *display);

	private:
		// Create.
		EglDevice(Engine &e, EGLDisplay display);

		// Initialization.
		bool init();

		// The display.
		EGLDisplay display;

		// The context.
		EGLContext context;

		// Configuration used for this context.
		EGLConfig config;
	};

}
#endif
