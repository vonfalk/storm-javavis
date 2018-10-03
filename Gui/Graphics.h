#pragma once
#include "Core/TObject.h"
#include "Core/Array.h"
#include "RenderInfo.h"
#include "Font.h"

namespace gui {
	class Painter;
	class Brush;
	class Text;
	class Path;
	class Bitmap;

	/**
	 * Generic interface for drawing somewhere.
	 *
	 * Created automatically if used with a painter. See the WindowGraphics class for the
	 * implementation.
	 */
	class Graphics : public ObjectOn<Render> {
		STORM_CLASS;
	public:
		// Create.
		STORM_CTOR Graphics();

		/**
		 * General format. Use push and pop to save/restore the state.
		 */

		// Clear all state on the stack.
		virtual void STORM_FN reset();

		// Push the current state on the state stack.
		virtual void STORM_FN push();

		// Push the current state with a modified opacity applied to the new state.
		virtual void STORM_FN push(Float opacity);

		// Push the current state with a clipping rectangle applied to the new state.
		virtual void STORM_FN push(Rect clip);

		// Push current state, clipping and opacity.
		virtual void STORM_FN push(Rect clip, Float opacity);

		// Pop the previous state. Returns false if nothing more to pop.
		virtual Bool STORM_FN pop();

		// Set the transform (in relation to the previous state).
		virtual void STORM_ASSIGN transform(Transform *tfm);

		// Set the line width (in relation to the previous state).
		virtual void STORM_ASSIGN lineWidth(Float w);

		/**
		 * Draw stuff.
		 */

		// Draw a line.
		virtual void STORM_FN line(Point from, Point to, Brush *brush);

		// Draw a rectangle.
		virtual void STORM_FN draw(Rect rect, Brush *brush);

		// Draw rounded rectangle.
		virtual void STORM_FN draw(Rect rect, Size edges, Brush *brush);

		// Draw an oval.
		virtual void STORM_FN oval(Rect rect, Brush *brush);

		// Draw a path.
		virtual void STORM_FN draw(Path *path, Brush *brush);

		// Fill a rectangle.
		virtual void STORM_FN fill(Rect rect, Brush *brush);

		// Fill rounded rectangle.
		virtual void STORM_FN fill(Rect rect, Size edges, Brush *brush);

		// Fill the entire area.
		virtual void STORM_FN fill(Brush *brush);

		// Fill a path.
		virtual void STORM_FN fill(Path *path, Brush *brush);

		// Fill an oval.
		virtual void STORM_FN fillOval(Rect rect, Brush *brush);

		// Draw a bitmap.
		void STORM_FN draw(Bitmap *bitmap);
		void STORM_FN draw(Bitmap *bitmap, Point topLeft);
		void STORM_FN draw(Bitmap *bitmap, Point topLeft, Float opacity);
		void STORM_FN draw(Bitmap *bitmap, Rect rect);
		virtual void STORM_FN draw(Bitmap *bitmap, Rect rect, Float opacity);

		// Draw a part of a bitmap.
		void STORM_FN draw(Bitmap *bitmap, Rect src, Point topLeft);
		void STORM_FN draw(Bitmap *bitmap, Rect src, Point topLeft, Float opacity);
		void STORM_FN draw(Bitmap *bitmap, Rect src, Rect dest);
		virtual void STORM_FN draw(Bitmap *bitmap, Rect src, Rect dest, Float opacity);

		// TODO: Add support for drawing a part of a bitmap in a convenient way. Eg. tile or similarly.

		// Draw text the easy way. Prefer using a Text object if possible, since they give more
		// control over the text and yeild better performance. Attempts to fit the given text inside
		// 'rect'. No attempt is made at clipping anything not fitting inside 'rect', so any text
		// not fitting inside 'rect' will be drawn outside the rectangle.
		virtual void STORM_FN text(Str *text, Font *font, Brush *brush, Rect rect);

		// Draw pre-formatted text.
		virtual void STORM_FN draw(Text *text, Brush *brush, Point origin);
	};


}
