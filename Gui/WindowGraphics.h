#pragma once
#include "Graphics.h"

namespace gui {

	/**
	 * Graphics object used for drawing stuff inside a Painter.
	 *
	 * Not usable outside the 'render' function inside a Painter.
	 */
	class WindowGraphics : public Graphics {
		STORM_CLASS;
	public:
		// Create. Done from within the Painter class. Will not free 'target'.
		WindowGraphics(RenderInfo info, Painter *owner);

		// Update the target.
		void updateTarget(RenderInfo info);

		// Destroy.
		~WindowGraphics();

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
		RenderInfo info;

		// Owner.
		Painter *owner;

		// A layer in D2D/OpenGL.
		class Layer {
			STORM_VALUE;
		public:
			Layer() {
				v = 0;
			}

			Layer(OsLayer layer) {
				v = layer;
			}

			// Return a dummy value usable for non-pointers. Used in D2D to indicate PushAxisAlignedClip.
			static Layer dummy() {
				return Layer((OsLayer)1);
			}

			inline operator bool() {
				return v != OsLayer(0);
			}

			inline bool operator ==(const Layer &o) const {
				return v == o.v;
			}

			inline bool operator !=(const Layer &o) const {
				return v != o.v;
			}

			UNKNOWN(PTR_NOGC) OsLayer v;

			// Release.
			void release();
		};

		// State. The values here are always absolute, ie they do not depend on
		// previous states on the state stack.
		class State {
			STORM_VALUE;
		public:
#ifdef GUI_WIN32
			State() {
				lineWidth = 1.0f;
				opacity = 1.0f;
				*transform() = dxUnit();
			}

			State(const D2D1_MATRIX_3X2_F &tfm, Float lineWidth) {
				*transform() = tfm;
				this->lineWidth = lineWidth;
				layer = null;
			}

			// Get the real type of the transformation matrix.
			inline D2D1_MATRIX_3X2_F *transform() {
				return (D2D1_MATRIX_3X2_F *)&tfm0;
			}
#endif
#ifdef GUI_GTK
			State() {
				lineWidth = 1.0f;
				opacity = 1.0f;
				tfm0 = 1; tfm1 = 0;
				tfm2 = 0; tfm3 = 1;
				tfm4 = 0; tfm5 = 0;
				layer = LayerKind::none;
			}

			State(const cairo_matrix_t &tfm, Float lineWidth) {
				this->lineWidth = lineWidth;
				transform(tfm);
			}

			// Set the transform.
			void transform(const cairo_matrix_t &src) {
				tfm0 = float(src.xx); tfm1 = float(src.yx);
				tfm2 = float(src.xy); tfm3 = float(src.yy);
				tfm4 = float(src.x0); tfm5 = float(src.y0);
			}

			// Get the transform.
			cairo_matrix_t transform() const {
				cairo_matrix_t r = {
					tfm0, tfm1,
					tfm2, tfm3,
					tfm4, tfm5
				};
				return r;
			}

#endif

			// Transform storage. Do not touch!
			Float tfm0;
			Float tfm1;
			Float tfm2;
			Float tfm3;
			Float tfm4;
			Float tfm5;

			// Clip region. Coordinates are in the space of the previous state's transform.
			Rect clip;

			// Line size.
			Float lineWidth;

			// Opacity (if needed by the backend).
			Float opacity;

			// Layer pushed with this state. May be null.
			Layer layer;
		};

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

#ifdef GUI_WIN32
		// Get a layer.
		ID2D1Layer *layer();
#endif

		// Prepare rendering a new frame (backend specific).
		void prepare();
	};


}
