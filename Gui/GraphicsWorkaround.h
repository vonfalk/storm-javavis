#pragma once
#include "WindowGraphics.h"
#include "OS/Stack.h"

namespace gui {

	/**
	 * Graphics object that is used to work around bugs in the graphics driver.
	 *
	 * In particular: the Iris driver for Intel cards contained a bug that caused it to accidentally
	 * store pointers to stack-allocated values too long, and thus crash since we deallocate old
	 * stacks from time to time.
	 *
	 * This class wraps the drawing calls of the Graphcis object and makes sure they are executed on
	 * a pre-determined stack.
	 *
	 * TODO: We might want to have a later version of this object be more like a "decorator" for
	 * some other class. This is a proof of concept version until the Gui rewrite is done.
	 */
	class WorkaroundGraphics : public WindowGraphics {
		STORM_CLASS;
	public:
		// Create. Done from within the Painter class. Will not free 'target'.
		WorkaroundGraphics(RenderInfo info, Painter *owner);

		// Destroy.
		~WorkaroundGraphics();

		// Prepare the rendering.
		void beforeRender();

		// Do any housekeeping after rendering.
		void afterRender();

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
		// The stack we shall execute all drawing code on.
		// TODO: We probably want to have a global single instance of this!
		os::Stack *stack;
	};


}
