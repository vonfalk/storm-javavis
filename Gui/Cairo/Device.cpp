#include "stdafx.h"
#include "Device.h"
#include "Graphics.h"
#include "RenderMgr.h"
#include "Exception.h"
#include "App.h"
#include "Window.h"
#include "Core/Convert.h"

#ifdef GUI_GTK

namespace gui {

	// Get the draw widget for a window:
	static GtkWidget *drawWidget(Engine &e, Handle handle) {
		Window *w = app(e)->findWindow(handle);
		if (!w)
			return handle.widget();
		else
			return w->drawWidget();
	}

	CairoDevice::CairoDevice(Engine &e) : e(e), myId(0) {}

	Nat CairoDevice::id() {
		if (myId == 0)
			myId = renderMgr(e)->allocId();
		return myId;
	}

	CairoSwDevice::CairoSwDevice(Engine &e) : CairoDevice(e) {}

	Surface *CairoSwDevice::createSurface(Handle window) {
		Size size(gtk_widget_get_allocated_width(window.widget()),
				gtk_widget_get_allocated_height(window.widget()));
		cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_RGB24, size.w, size.h);

		return new CairoBlitSurface(id(), size, surface);
	}


	CairoGtkDevice::CairoGtkDevice(Engine &e) : CairoDevice(e) {}

	Surface *CairoGtkDevice::createSurface(Handle window) {
		GdkWindow *win = gtk_widget_get_window(drawWidget(e, window));
		if (!win)
			return null;

		Size size(gtk_widget_get_allocated_width(window.widget()),
				gtk_widget_get_allocated_height(window.widget()));

		return new CairoBlitSurface(id(), size,
									gdk_window_create_similar_surface(win, CAIRO_CONTENT_COLOR, int(size.w), int(size.h)));
	}


	CairoSurface::CairoSurface(Nat id, Size size)
		: Surface(size, 1.0f), surface(null), device(null), id(id) {}


	CairoSurface::CairoSurface(Nat id, Size size, cairo_surface_t *surface)
		: Surface(size, 1.0f), surface(surface), device(null), id(id) {

		if (surface)
			device = cairo_create(surface);
	}

	WindowGraphics *CairoSurface::createGraphics(Engine &e) {
		return new (e) CairoGraphics(*this, id, false);
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


	CairoBlitSurface::CairoBlitSurface(Nat id, Size size, cairo_surface_t *surface)
		: CairoSurface(id, size, surface) {}

	Surface::PresentStatus CairoBlitSurface::present(bool waitForVSync) {
		// We cannot do much here...
		return pRepaint;
	}

	void CairoBlitSurface::repaint(RepaintParams *params) {
		cairo_set_source_surface(params->cairo, surface, 0, 0);
		cairo_paint(params->cairo);

		// Make sure the operation is not pending, this function is called inside a lock.
		cairo_surface_flush(cairo_get_group_target(params->cairo));
	}

}

#endif
