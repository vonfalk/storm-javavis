#pragma once
#include "Gui/Device.h"
#include "Gui/Surface.h"

#ifdef GUI_GTK

namespace gui {

	/**
	 * Generic Cairo device. Used to coordinate ID creation.
	 */
	class CairoDevice : public Device {
	public:
		// Create.
		CairoDevice(Engine &e);

	protected:
		// Get the ID.
		Nat id();

	private:
		// Engine.
		Engine &e;

		// Our ID.
		Nat myId;
	};

	/**
	 * Cairo software device.
	 */
	class CairoSwDevice : public CairoDevice {
	public:
		// Create.
		CairoSwDevice(Engine &e);

		// Create a surface.
		virtual Surface *createSurface(Handle window);
	};


	/**
	 * Cairo device that uses whatever Gtk+ suggests (Usually an XLib surface).
	 */
	class CairoGtkDevice : public CairoDevice {
	public:
		// Create.
		CairoGtkDevice(Engine &e);

		// Create a surface.
		virtual Surface *createSurface(Handle window);
	};


	/**
	 * Cairo device that uses OpenGL. Note: Since bitmaps won't (likely) be shareable between
	 * different instances, we generate unique IDs for each surface here.
	 */
	class CairoGlDevice : public Device {
	public:
		// Create.
		CairoGlDevice(Engine &e);

		// Create a surface.
		virtual Surface *createSurface(Handle window);

	private:
		// Engine.
		Engine &e;
	};


	/**
	 * Generic Cairo surface.
	 */
	class CairoSurface : public Surface {
	public:
		// Create.
		CairoSurface(Nat id, Size size);
		CairoSurface(Nat id, Size size, cairo_surface_t *surface);

		// The cairo device.
		cairo_t *device;

		// The cairo surface.
		cairo_surface_t *surface;

		// Create a Graphics object.
		virtual WindowGraphics *createGraphics(Engine &e);

		// Resize the surface.
		virtual void resize(Size size, Float scale);

	private:
		// Device ID.
		Nat id;
	};


	/**
	 * Cairo surface that uses blitting to present to the window.
	 */
	class CairoBlitSurface : public CairoSurface {
	public:
		// Create.
		CairoBlitSurface(Nat id, Size size, cairo_surface_t *surface);

		// Present.
		virtual PresentStatus present(bool waitForVSync);

		// Paint.
		virtual void repaint(RepaintParams *params);
	};


	/**
	 * Cairo GL surface.
	 */
	class CairoGlSurface : public CairoSurface {
	public:
		// Create.
		CairoGlSurface(Nat id, Size size, GdkWindow *window);

		// Destroy.
		~CairoGlSurface();

		// Present.
		virtual PresentStatus present(bool waitForVSync);

		// Paint.
		virtual void repaint(RepaintParams *params);

		// Resize the surface.
		virtual void resize(Size size, Float scale);

	private:
		// Gl context.
		GdkGLContext *context;

		// Main texture.
		GLuint texture;

		// Cairo device.
		cairo_device_t *dev;
	};

}

#endif
