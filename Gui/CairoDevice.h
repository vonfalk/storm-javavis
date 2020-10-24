#pragma once
#include "RenderInfo.h"
#include "Handle.h"
#include "Device.h"

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

		// Get the device type (always some kind of blitting, as Gtk+ likes to keep track of the window data).
		DeviceType type() { return dtBlit; }

		// Attach a painter. If it returns a RenderInfo where 'any()' returns false, call 'create' later.
		RenderInfo attach(Handle window);

		// Resize the target of a painter.
		void resize(RenderInfo &info, Size size);

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

		// Draw this surface to the supplied cairo_t in a suitable way. Will do a "paint" operation,
		// so assumes that "to" is properly set up.
		virtual void blit(cairo_t *to);

		// Do we need to flip the Y axis on this surface?
		virtual bool flipY() const { return false; }

	protected:
		// Create when initializing 'surface' at a later point.
		CairoSurface() {}
	};


	/**
	 * A generic representation of a Cairo backend. This essentially describes how to create new Cairo surfaces.
	 */
	class CairoDevice : NoCopy {
	public:
		// Create a surface for a window.
		virtual CairoSurface *createSurface(Size size, Handle window) = 0;

		// Create a surface for Pango font rendering. Like 'createSurface', but does not create
		// require a window.
		virtual CairoSurface *createPangoSurface(Size size);
	};

	/**
	 * A fully software rendered Cairo backend.
	 */
	class SoftwareDevice : public CairoDevice {
	public:
		// This never fails in this implementation, as we can simply create the device at any time.
		virtual CairoSurface *createSurface(Size size, Handle window);
	};

	/**
	 * A backend that re-uses whatever backend Gtk+ is using. This means that we need to delay
	 * creation of surfaces until painting is actually done.
	 */
	class GtkDevice : public CairoDevice {
	public:
		// Create for a window.
		virtual CairoSurface *createSurface(Size size, Handle window);
	};

	/**
	 * GL surface.
	 *
	 * Handles an OpenGL instance associated with the window, and a created texture to which Cairo
	 * can draw. We then use the appropriate Cairo+GL integration to copy the texture into the
	 * surface efficiently.
	 */
	class GlSurface : public CairoSurface {
	public:
		// Create.
		GlSurface(GdkWindow *window, Size size);

		// Destroy.
		virtual ~GlSurface();

		// Resize the surface.
		virtual void resize(Size size);

		// Draw the surface. Uses gdk_cairo_draw_from_gl, which is supposed to be efficient for this.
		virtual void blit(cairo_t *to);

		// We need to flip the Y axis...
		virtual bool flipY() const { return true; }

	private:
		// GL context.
		GdkGLContext *context;

		// Cairo GL device.
		cairo_device_t *device;

		// Current size of the texture.
		Nat width;
		Nat height;

		// GL texture id.
		GLuint texture;
	};

	/**
	 * GL device.
	 *
	 * Uses Cairo in OpenGL mode for hardware assisted drawing. This is noticeably faster in cases
	 * with many large-ish bitmaps. Otherwise the GtkDevice backend is often good enough, as it
	 * (supposedly) is able to accelerate some operations through the X server, and whatever backend
	 * is the default.
	 */
	class GlDevice : public CairoDevice {
	public:
		// Create for a window.
		virtual CairoSurface *createSurface(Size size, Handle window);
	};

}
#endif
