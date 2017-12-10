#pragma once

#ifdef GUI_GTK

#include <poll.h>
#include <X11/Xlib.h>
#include <gtk/gtk.h>
#include <gtk/gtkx.h>
#include <GL/gl.h>
#include <GL/glx.h>
#include <EGL/egl.h>
#include "GtkLayout.h"

// Some headers define 'Bool' for us. That is not good...
#undef Bool

namespace gui {

	/**
	 * Define generic types to be used in Storm class declarations.
	 */
	typedef cairo_pattern_t OsResource;
	typedef PangoLayout OsTextLayout;

	// More convenient matrix operations.
	cairo_matrix_t cairoMultiply(const cairo_matrix_t &a, const cairo_matrix_t &b);
	cairo_matrix_t cairo(Transform *tfm);

	// Convert to/from Pango units.
	inline int toPango(float val) { return int(val * PANGO_SCALE); }
	inline float fromPango(int val) { return float(val) / float(PANGO_SCALE); }
}

#else

// Allow using some types in class declarations even if we're not using Gtk+.
struct cairo_t;
struct cairo_surface_t;
struct PangoContext;
struct GdkWindow;

#endif
