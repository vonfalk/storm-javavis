#pragma once
#include "Gui/Device.h"
#include "Gui/Surface.h"

#ifdef GUI_GTK

namespace gui {

	/**
	 * Cairo software device.
	 */
	class CairoSwDevice : public Device {
	public:
		// Create.
		CairoSwDevice(Engine &e);

		// Create a surface.
		virtual Surface *createSurface(Handle window);
	};


	/**
	 * Cairo device that uses whatever Gtk+ suggests (Usually an XLib surface).
	 */
	class CairoSwDevice : public Device {
	public:
		// Create.
		CairoSwDevice(Engine &e);

		// Create a surface.
		virtual Surface *createSurface(Handle window);
	};


	/**
	 * Cairo device that uses OpenGL.
	 */
	class CairoGLDevice : public Device {
	public:
		// Create.
		CairoGLDevice(Engine &e);

		// Create a surface.
		virtual Surface *createSurface(Handle window);
	};


	/**
	 * Generic Cairo surface.
	 */
	class CairoSurface : public Surface {
	public:
		// Create.
		CairoSurface(Size size, cairo_surface_t *surface);

		// The cairo device.
		cairo_t *device;

		// The cairo surface.
		cairo_surface_t *surface;

		// Create a Graphics object.
		virtual WindowGraphics *createGraphics(Engine &e);

		// Resize the surface.
		void resize(Size size, Float scale);
	};


	/**
	 * Cairo surface that uses blitting to present to the window.
	 */
	class CairoBlitSurface : public Surface {
	public:
		// Create.
		CairoBlitSurface(Size size, cairo_surface_t *surface);

		// Present.
		virtual bool present(bool waitForVSync);
	};

}

#endif
