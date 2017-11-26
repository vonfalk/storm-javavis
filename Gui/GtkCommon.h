#pragma once

#ifdef GUI_GTK

#include <gtk/gtk.h>
#include <cairo/cairo-gl.h>
#include "GtkLayout.h"

#include <EGL/egl.h>
#include <GL/gl.h>
// The macro 'Bool' collides with our type...
#undef Bool


#if defined(GDK_WINDOWING_X11)
#include <gdk/gdkx.h>
#elif defined(GDK_WINDOWING_WAYLAND)
#include <gdk/gdxwayland.h>
#else
#error "Unknown windowing system used."
#endif


namespace gui {

	// Nothing here yet...

}

#endif
