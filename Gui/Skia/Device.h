#pragma once
#include "Gui/Device.h"
#include "Gui/Surface.h"
#include "Skia.h"

#ifdef GUI_GTK

namespace gui {

	/**
	 * Rendering context for Skia.
	 */
	class SkiaContext : public GLDevice::Context {
	public:
		// Create.
		SkiaContext(GLDevice *owner, GdkWindow *window, GdkGLContext *context);

		// Destroy.
		~SkiaContext();

		// Skia context.
		sk_sp<GrDirectContext> skia;
	};

	/**
	 * A Skia device.
	 */
	class SkiaDevice : public GLDevice {
	public:
		// Create.
		SkiaDevice(Engine &e);

		// Create a text manager.
		virtual TextMgr *createTextMgr();

	protected:
		// Create a context.
		virtual SkiaContext *createContext(GdkWindow *window, GdkGLContext *context);

		// Create a surface.
		virtual Surface *createSurface(GtkWidget *widget, Context *context);
	};


	/**
	 * A Skia surface.
	 */
	class SkiaSurface : public Surface {
	public:
		// Create.
		SkiaSurface(Size size, SkiaContext *context);

		// Destroy.
		~SkiaSurface();

		// Present.
		virtual PresentStatus present(bool waitForVSync);

		// Paint.
		virtual void repaint(RepaintParams *params);

		// Resize the surface.
		virtual void resize(Size size, Float scale);

		// Create a Graphics object.
		virtual WindowGraphics *createGraphics(Engine &e);

		// Canvas for the surface (a mirror of surface->getCanvas()). Updated whenever needed.
		SkCanvas *canvas;

		// Skia surface.
		sk_sp<SkSurface> surface;

	private:
		// Context.
		SkiaContext *context;

		// Frame buffer object.
		GLuint framebuffer;

		// Color buffer we're rendering to.
		GLuint colorbuffer;

		// Stencil buffer.
		GLuint stencilbuffer;

		// Backend render target. Must be kept alive as long as the surface is alive.
		GrBackendRenderTarget target;
	};

}

#endif
