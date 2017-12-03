#pragma once

#ifdef GUI_GTK

#include <poll.h>
#include <X11/Xlib.h>
#include <gtk/gtk.h>
#include "GtkLayout.h"

// Some headers define 'Bool' for us. That is not good...
#undef Bool

namespace gui {

	/**
	 * Define generic types to be used in Storm class declarations.
	 */
	typedef cairo_pattern_t OsResource;


	// More convenient matrix operations.
	cairo_matrix_t cairoMultiply(const cairo_matrix_t &a, const cairo_matrix_t &b);
	cairo_matrix_t cairo(Transform *tfm);

}

#endif
