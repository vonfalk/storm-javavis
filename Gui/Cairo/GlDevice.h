#pragma once
#include "Device.h"

#ifdef GUI_GTK

namespace gui {

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

		// Create a text manager.
		virtual TextMgr *createTextMgr();

	private:
		// Engine.
		Engine &e;
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

		// Create a Graphics object with flipped Y coordinates.
		virtual WindowGraphics *createGraphics(Engine &e);

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
