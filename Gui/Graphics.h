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
	 * Graphics object used for drawing stuff inside a Painter.
	 * Not usable outside the 'render' function inside a Painter.
	 */
	class Graphics : public ObjectOn<Render> {
		STORM_CLASS;
	public:
		// Create. Done from within the Painter class. Will not free 'target'.
		Graphics(RenderInfo info, Painter *owner);

		// Update the target.
		void updateTarget(RenderInfo info);

		// Destroy.
		~Graphics();

		// Get the drawing size.
		Size size();

		// Called when the target is destroyed.
		void destroyed();

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
		void STORM_SETTER transform(Transform *tfm);

		// Set the line width (in relation to the previous state).
		void STORM_SETTER lineWidth(Float w);

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
		void STORM_FN draw(Bitmap *bitmap);
		void STORM_FN draw(Bitmap *bitmap, Point topLeft);
		void STORM_FN draw(Bitmap *bitmap, Point topLeft, Float opacity);
		void STORM_FN draw(Bitmap *bitmap, Rect rect);
		void STORM_FN draw(Bitmap *bitmap, Rect rect, Float opacity);

		// Draw text.
		void STORM_FN text(Str *text, Font *font, Brush *brush, Rect rect);

		// Draw pre-formatted text.
		void STORM_FN draw(Text *text, Brush *brush, Point origin);

	private:
		// Render target.
		RenderInfo info;

		// Owner.
		Painter *owner;

		// A layer in D2D.
		class Layer {
			STORM_VALUE;
		public:
			Layer() {
				v = null;
			}

			// Layer(ID2D1Layer *layer) {
			// 	v = layer;
			// }

			// UNKNOWN(PTR_NOGC) ID2D1Layer *v;
			UNKNOWN(PTR_NOGC) void *v;

			// Release.
			void release();
		};

		// State. The values here are always absolute, ie they do not depend on
		// previous states on the state stack.
		class State {
			STORM_VALUE;
		public:
			State() {}

			// State(const D2D1_MATRIX_3X2_F &tfm, Float lineWidth) {
			// 	*transform() = tfm;
			// 	this->lineWidth = lineWidth;
			// 	layer = null;
			// }

			// Transform storage. Do not touch!
			Float tfm0;
			Float tfm1;
			Float tfm2;
			Float tfm3;
			Float tfm4;
			Float tfm5;

			// Get the real type.
			// inline D2D1_MATRIX_3X2_F *transform() {
			// 	return (D2D1_MATRIX_3X2_F *)&tfm0;
			// }

			// Line size.
			Float lineWidth;

			// Layer pushed with this state. May be null.
			Layer layer;
		};

		// Default state.
		State defaultState();

		// State stack. Always contains at least one element (the default state).
		Array<State> *oldStates;

		// Current state.
		State state;

		// # of layers to remember.
		enum {
			layerHistoryCount = 10,
		};

		// Keep track of how many layers we have used the last few frames, so that we can destroy
		// unneeded layers if they are not used.
		GcArray<Nat> *layerHistory;

		// Minimum number of free layers during the rendering.
		nat minFreeLayers;

		// Keep track of layers used.
		Array<Layer> *layers;

		// Get a layer.
		// ID2D1Layer *layer();
	};

}
