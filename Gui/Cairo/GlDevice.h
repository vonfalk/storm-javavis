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

		// Destroy.
		~CairoGlDevice();

		// Create a surface.
		virtual Surface *createSurface(Handle window);

		// Called by GL surfaces when they are destroyed to indicate that they no longer need the GL context.
		void unrefContext();

		// GL context. Might be null.
		GdkGLContext *context;

		// Cairo device. Might be null.
		cairo_device_t *device;

	private:
		// Engine.
		Engine &e;

		// Users of the GL device and context.
		Nat surfaces;

		// Our ID.
		Nat id;

		// Create the context.
		void create(GdkWindow *window);

		// Destroy the context.
		void destroy();
	};


	/**
	 * Cairo GL surface.
	 */
	class CairoGlSurface : public CairoSurface {
	public:
		// Create.
		CairoGlSurface(Nat id, Size size, CairoGlDevice &owner);

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
		CairoGlDevice &owner;

		// Main texture.
		GLuint texture;
	};

}

#endif
