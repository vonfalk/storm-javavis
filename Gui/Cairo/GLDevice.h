#pragma once
#include "Device.h"

#ifdef GUI_GTK

namespace gui {

	/**
	 * Our specialization of a GLDevice::Context. Also contains a Cairo device.
	 */
	class CairoGLContext : public GLDevice::Context {
	public:
		// Create.
		CairoGLContext(GLDevice *owner, GdkWindow *window, GdkGLContext *context);

		// Destroy.
		~CairoGLContext();

		// The cairo device.
		cairo_device_t *device;
	};


	/**
	 * Cairo device that uses OpenGL. Note: Since bitmaps won't (likely) be shareable between
	 * different instances, we generate unique IDs for each surface here.
	 */
	class CairoGLDevice : public GLDevice {
	public:
		// Create.
		CairoGLDevice(Engine &e);

		// Create a text manager.
		virtual TextMgr *createTextMgr();

		// We're hardware accelerated.
		virtual bool isHardware() const { return true; }

	protected:
		// Create a context.
		virtual CairoGLContext *createContext(GdkWindow *window, GdkGLContext *context);

		// Create a surface.
		virtual Surface *createSurface(GtkWidget *widget, Context *context);
	};


	/**
	 * Cairo GL surface.
	 */
	class CairoGLSurface : public CairoSurface {
	public:
		// Create.
		CairoGLSurface(Size size, CairoGLContext *context);

		// Destroy.
		~CairoGLSurface();

		// Present.
		virtual PresentStatus present(bool waitForVSync);

		// Create a Graphics object with flipped Y coordinates.
		virtual WindowGraphics *createGraphics(Engine &e);

		// Paint.
		virtual void repaint(RepaintParams *params);

		// Resize the surface.
		virtual void resize(Size size, Float scale);

	private:
		// GL context.
		CairoGLContext *context;

		// Main texture.
		GLuint texture;
	};

}

#endif
