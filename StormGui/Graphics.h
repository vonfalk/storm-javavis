#pragma once
#include "Brush.h"
#include "Bitmap.h"

namespace stormgui {
	class Painter;

	/**
	 * Graphics object used for drawing stuff inside a Painter.
	 * Not usable outside the 'render' function inside a Painter.
	 */
	class Graphics : public ObjectOn<Render> {
		STORM_CLASS;
	public:
		// Create. Done from within the Painter class. Will not free 'target'.
		Graphics(ID2D1RenderTarget *target, Painter *owner);

		// Destroy.
		~Graphics();

		// Get the drawing size.
		Size size();

		// Called when the target is destroyed.
		void destroyed();

		// Update the target.
		void updateTarget(ID2D1RenderTarget *target);


		/**
		 * Draw stuff.
		 */

		// Draw a line.
		void STORM_FN line(Point from, Point to, Par<Brush> brush);

		// Draw a rectangle.
		void STORM_FN rect(Rect rect, Par<Brush> brush);

		// Draw rounded rectangle.
		void STORM_FN rect(Rect rect, Size edges, Par<Brush> brush);

		// Draw an oval.
		void STORM_FN oval(Rect rect, Par<Brush> brush);

		// Fill a rectangle.
		void STORM_FN fillRect(Rect rect, Par<Brush> brush);

		// Fill rounded rectangle.
		void STORM_FN fillRect(Rect rect, Size edgex, Par<Brush> brush);

		// Fill an oval.
		void STORM_FN fillOval(Rect rect, Par<Brush> brush);

		// Draw a bitmap.
		void STORM_FN draw(Par<Bitmap> bitmap, Point topLeft);
		void STORM_FN draw(Par<Bitmap> bitmap, Rect rect);

	private:
		// Render target.
		ID2D1RenderTarget *target;

		// Owner.
		Painter *owner;
	};

}
