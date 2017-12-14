#pragma once

#ifdef GUI_GTK

#include <poll.h>
#include <X11/Xlib.h>
#define GL_GLEXT_PROTOTYPES
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
	typedef void *OsResource;
	typedef PangoLayout OsTextLayout;

	// Convert to/from Pango units.
	inline int toPango(float val) { return int(val * PANGO_SCALE); }
	inline float fromPango(int val) { return float(val) / float(PANGO_SCALE); }
}

#else

// Allow using some types in class declarations even if we're not using Gtk+.
struct PangoContext;
struct GdkWindow;

#endif
