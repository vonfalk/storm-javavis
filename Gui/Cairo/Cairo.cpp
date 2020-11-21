#include "stdafx.h"
#include "Cairo.h"
#include "Graphics.h"

#ifdef GUI_GTK

namespace gui {

	CairoSwDevice::CairoSwDevice(Engine &) {}

	Surface *CairoSwDevice::createSurface(Handle window) {
		Size size(gtk_widget_get_allocated_width(window.widget()),
				gtk_widget_get_allocated_height(window.widget()));
		cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_RGB24, size.w, size.h);

		return new CairoBlitSurface(size, surface);
	}


	CairoGtkSurface::CairoGtkSurface(Engine &) {}

	Surface *CairoGtkSurface::createSurface(Handle window) {
		GdkWindow *win = gtk_widget_get_window(window.handle());
		if (!win)
			return null;

		Size size(gtk_widget_get_allocated_width(window.widget()),
				gtk_widget_get_allocated_height(window.widget()));

		return new CairoBlitSurface(size, gdk_window_create_similar_surface(win, CAIRO_CONTENT_COLOR, int(size.w), int(size.h)));
	}


	CairoSurface::CairoSurface(Size size, cairo_surface_t *surface)
		: Surface(size, 1.0f), surface(surface) {

		device = cairo_create(surface);
	}

	WindowGraphics *CairoSurface::createGraphics(Engine &e) {
		return new (e) CairoGraphics(*this);
	}

	void CairoSurface::resize(Size size, Float scale) {
		this->scale = scale;
		this->size = size;

		cairo_destroy(device);
		cairo_surface_t *tmp = cairo_surface_create_similar(surface, CAIRO_CONTENT_COLOR, int(size.w), int(size.h));
		cairo_surface_destroy(surface);
		surface = tmp;
		device = cairo_create(surface);
	}


	CairoBlitSurface::CairoBlitSurface(Size size, cairo_surface_t *surface)
		: CairoSurface(size, surface) {}

	bool CairoBlitSurface::present(bool waitForVSync) {}

}

#endif
