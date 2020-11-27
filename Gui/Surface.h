#pragma once
#include "WindowGraphics.h"

namespace gui {

	/**
	 * Parameters passed to "repaint" calls. Highly backend-specific.
	 */
	struct RepaintParams {
#ifdef GUI_GTK
		GdkWindow *window;
		GtkWidget *widget;
		cairo_t *cairo;
#endif
	};

	/**
	 * A generic surface associated with some window on the current system, to which it is possible
	 * to draw.
	 *
	 * All functions here are called from the Render thread, except "repaint", which is called from
	 * the UI thread.
	 */
	class Surface : NoCopy {
	public:
		// Create.
		Surface(Size size, Float scale) : size(size), scale(scale) {}

		// Size of the drawing area.
		Size size;

		// Scale (to respect DPI).
		Float scale;

		// Create a Graphics object for this surface.
		virtual WindowGraphics *createGraphics(Engine &e) = 0;

		// Resize this surface.
		virtual void resize(Size size, Float scale) = 0;

		// Status codes from "present".
		enum PresentStatus {
			// All went well:
			pSuccess,

			// Re-create the associated resources and try again:
			pRecreate,

			// We cannot present from the rendering thread. Ask the UI thread to repaint the
			// associated window and call "repaint" from the UI thread.
			pRepaint,

			// Fatal failure.
			pFailure,
		};

		// Try to present this surface directly from the Render thread.
		virtual PresentStatus present(bool waitForVSync) = 0;

		// Called from the UI thread when "present" returned "repaint".
		// Default implementation provided as all subclasses do not need this operation.
		virtual void repaint(RepaintParams *params) {
			(void)params;
		}
	};

}
