#pragma once
#include "Gui/WindowGraphics.h"
#include "Skia.h"

namespace gui {

	STORM_PKG(impl);

	class SkiaSurface;

	/**
	 * Graphics object used when drawing using Skia on Linux.
	 *
	 * This usually means that we are using some kind of software rendering (either to X11 surfaces
	 * or entirely in software). It is possible to draw to OpenGL using Skia, but that seems a bit
	 * shaky, especially since we're using multithreading.
	 */
	class SkiaGraphics : public WindowGraphics {
		STORM_CLASS;
	public:
		// Create.
		SkiaGraphics(SkiaSurface &surface, Nat id);

		// Destroy.
		~SkiaGraphics();

		// Called when the surface was resized.
		void surfaceResized();

		// Called when the target is destroyed.
		void surfaceDestroyed();

		// Prepare the rendering.
		void beforeRender(Color bgColor);

		// Do any housekeeping after rendering.
		bool afterRender();

		/**
		 * General format. Use push and pop to save/restore the state.
		 */

		// Clear all state on the stack.
		void STORM_FN reset();

		// Push the current state.
		void STORM_FN push();

		// Push the current state with a modified opacity.
		void STORM_FN push(Float opacity);

		// Push the current state with a clipping rectangle.
		void STORM_FN push(Rect clip);

		// Push current state, clipping and opacity.
		void STORM_FN push(Rect clip, Float opacity);

		// Pop the previous state. Returns false if nothing more to pop.
		Bool STORM_FN pop();

		// Set the transform (in relation to the previous state).
		void STORM_ASSIGN transform(Transform *tfm);

		// Set the line width (in relation to the previous state).
		void STORM_ASSIGN lineWidth(Float w);

		/**
		 * Draw stuff.
		 */

		// Draw a line.
		void STORM_FN line(Point from, Point to, Brush *brush);

		// Draw a rectangle.
		void STORM_FN draw(Rect rect, Brush *brush);

		// Draw rounded rectangle.
		void STORM_FN draw(Rect rect, Size edges, Brush *brush);

		// Draw an oval.
		void STORM_FN oval(Rect rect, Brush *brush);

		// Draw a path.
		void STORM_FN draw(Path *path, Brush *brush);

		// Fill a rectangle.
		void STORM_FN fill(Rect rect, Brush *brush);

		// Fill rounded rectangle.
		void STORM_FN fill(Rect rect, Size edges, Brush *brush);

		// Fill the entire area.
		void STORM_FN fill(Brush *brush);

		// Fill a path.
		void STORM_FN fill(Path *path, Brush *brush);

		// Fill an oval.
		void STORM_FN fillOval(Rect rect, Brush *brush);

		// Draw a bitmap.
		void STORM_FN draw(Bitmap *bitmap, Rect rect, Float opacity);

		// Draw a part of a bitmap.
		void STORM_FN draw(Bitmap *bitmap, Rect src, Rect dest, Float opacity);

		// Draw text the easy way. Prefer using a Text object if possible, since they give more
		// control over the text and yeild better performance. Attempts to fit the given text inside
		// 'rect'. No attempt is made at clipping anything not fitting inside 'rect', so any text
		// not fitting inside 'rect' will be drawn outside the rectangle.
		void STORM_FN text(Str *text, Font *font, Brush *brush, Rect rect);

		// Draw pre-formatted text.
		void STORM_FN draw(Text *text, Brush *brush, Point origin);

	private:
		// Render target.
		SkiaSurface &surface;
	};

}
