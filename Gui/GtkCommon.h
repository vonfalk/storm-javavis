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

}

#endif
