#pragma once

#ifdef GUI_GTK

#include <poll.h>
#include <X11/Xlib.h>
#define GL_GLEXT_PROTOTYPES
#include <cairo/cairo.h>
#include "CairoGL.h"
#include <gtk/gtk.h>
#include <gtk/gtkx.h>
#include "GtkLayout.h"

// Some headers define 'Bool' for us. That is not good...
#undef Bool

namespace gui {

	class TextureContext;

	/**
	 * Define generic types to be used in Storm class declarations.
	 */
	typedef void OsResource;
	typedef size_t OsLayer;
	typedef PangoLayout OsTextLayout;

	/**
	 * Layer types in Cairo.
	 */
	enum STORM_HIDDEN(LayerKind) {
		none,
		group,
		save
	};

	/**
	 * How do we destroy various objects inside Cairo?
	 */
	template <class T>
	struct Destroy;

	template <>
	struct Destroy<cairo_pattern_t> {
		static void destroy(OsResource *p) {
			cairo_pattern_destroy((cairo_pattern_t *)p);
		}
	};

	template <>
	struct Destroy<cairo_surface_t> {
		static void destroy(OsResource *i) {
			cairo_surface_destroy((cairo_surface_t *)i);
		}
	};

	// More convenient matrix operations.
	cairo_matrix_t cairoMultiply(const cairo_matrix_t &a, const cairo_matrix_t &b);
	cairo_matrix_t cairo(Transform *tfm);

	// Convert to/from Pango units.
	inline int toPango(float val) { return int(ceilf(val * PANGO_SCALE)); }
	inline float fromPango(int val) { return float(val) / float(PANGO_SCALE); }
}

#else

// Allow using some types in class declarations even if we're not using Gtk+.
// TODO: Can we remove some of these?
struct PangoContext;
struct GdkWindow;
struct GdkDisplay;

#endif
