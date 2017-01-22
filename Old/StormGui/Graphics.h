#pragma once
#include "Brush.h"
#include "Bitmap.h"
#include "Font.h"
#include "Text.h"
#include "Path.h"

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

		// Set the transform (in relation to the previous state).
		void STORM_SETTER transform(Par<Transform> tfm);

		// Set the line width (in relation to the previous state).
		void STORM_SETTER lineWidth(Float w);

		/**
		 * Draw stuff.
		 */

		// Draw a line.
		void STORM_FN line(Point from, Point to, Par<Brush> brush);

		// Draw a rectangle.
		void STORM_FN draw(Rect rect, Par<Brush> brush);

		// Draw rounded rectangle.
		void STORM_FN draw(Rect rect, Size edges, Par<Brush> brush);

		// Draw an oval.
		void STORM_FN oval(Rect rect, Par<Brush> brush);

		// Draw a path.
		void STORM_FN draw(Par<Path> path, Par<Brush> brush);

		// Fill a rectangle.
		void STORM_FN fill(Rect rect, Par<Brush> brush);

		// Fill rounded rectangle.
		void STORM_FN fill(Rect rect, Size edges, Par<Brush> brush);

		// Fill the entire area.
		void STORM_FN fill(Par<Brush> brush);

		// Fill a path.
		void STORM_FN fill(Par<Path> path, Par<Brush> brush);

		// Fill an oval.
		void STORM_FN fillOval(Rect rect, Par<Brush> brush);

		// Draw a bitmap.
		void STORM_FN draw(Par<Bitmap> bitmap);
		void STORM_FN draw(Par<Bitmap> bitmap, Point topLeft);
		void STORM_FN draw(Par<Bitmap> bitmap, Point topLeft, Float opacity);
		void STORM_FN draw(Par<Bitmap> bitmap, Rect rect);
		void STORM_FN draw(Par<Bitmap> bitmap, Rect rect, Float opacity);

		// Draw text.
		void STORM_FN text(Par<Str> text, Par<Font> font, Par<Brush> brush, Rect rect);

		// Draw pre-formatted text.
		void STORM_FN draw(Par<Text> text, Par<Brush> brush, Point origin);


	private:
		// Render target.
		ID2D1RenderTarget *target;

		// Owner.
		Painter *owner;

		// State. The values here are always absolute, ie they do not depend on
		// previous states on the state stack.
		struct State {
			// Transform.
			D2D1_MATRIX_3X2_F transform;

			// Line size.
			float lineWidth;

			// Layer pushed with this state. May be null.
			ID2D1Layer *layer;
		};

		// default state.
		State defaultState();

		// State stack. Always contains at least one element (the default state).
		vector<State> oldStates;

		// Current state.
		State state;

		// # of layers to remember.
		enum {
			layerHistory = 10,
		};

		// Keep track of how many layers we have used the last few frames, so that we can destroy
		// unneeded layers if they are not used.
		nat layerCount[layerHistory];

		// Minimum number of free layers during the rendering.
		nat minFreeLayers;

		// Keep track of layers used.
		vector<ID2D1Layer *> layers;

		// Get a layer.
		ID2D1Layer *layer();
	};

}
