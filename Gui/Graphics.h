#pragma once
#include "Core/TObject.h"
#include "Core/Array.h"
#include "Core/WeakSet.h"
#include "Font.h"

namespace gui {
	class Painter;
	class Brush;
	class Text;
	class Path;
	class Bitmap;
	class Resource;
	class GraphicsResource;
	class SolidBrush;
	class LinearGradient;
	class RadialGradient;

	/**
	 * Generic interface for drawing somewhere.
	 *
	 * Created automatically if used with a painter. See the WindowGraphics class for the
	 * implementation.
	 *
	 * Each painter contains an identifier that is used to associate a graphics instance with
	 * resources used by it. This allows multiple Graphics objects for different backends to be
	 * active simultaneously, and even draw using the same set of graphics resources. Additionally,
	 * different backends have different characteristics regarding how they handle their
	 * resources. Some backends (e.g. software backends) do not require any particular
	 * representation of resources, others (e.g. D2D) can use one internal representation throughout
	 * all Graphics objects, and yet others (e.g. OpenGL-based backends on Gtk) require
	 * instace-specific representations. This is reflected in how identifiers are allocated.
	 */
	class Graphics : public ObjectOn<Render> {
		STORM_ABSTRACT_CLASS;
	public:
		// Create.
		STORM_CTOR Graphics();

		// Get the identifier for this Graphics object.
		Nat STORM_FN id() const { return identifier; }

		// Called when a resource tied to this graphics object is created. Returns 'true' if the
		// resource was newly added here, and false otherwise.
		Bool STORM_FN attach(Resource *resource);

		// Destroy all resources associated to this object.
		virtual void STORM_FN destroy();

		/**
		 * General format. Use push and pop to save/restore the state.
		 */

		// Clear all state on the stack.
		virtual void STORM_FN reset();

		// Push the current state on the state stack.
		virtual void STORM_FN push() ABSTRACT;

		// Push the current state with a modified opacity applied to the new state.
		virtual void STORM_FN push(Float opacity) ABSTRACT;

		// Push the current state with a clipping rectangle applied to the new state.
		virtual void STORM_FN push(Rect clip) ABSTRACT;

		// Push current state, clipping and opacity.
		virtual void STORM_FN push(Rect clip, Float opacity) ABSTRACT;

		// Pop the previous state. Returns false if nothing more to pop.
		virtual Bool STORM_FN pop() ABSTRACT;

		// Set the transform (in relation to the previous state).
		virtual void STORM_ASSIGN transform(Transform *tfm) ABSTRACT;

		// Set the line width (in relation to the previous state).
		virtual void STORM_ASSIGN lineWidth(Float w) ABSTRACT;

		/**
		 * Draw stuff.
		 */

		// Draw a line.
		virtual void STORM_FN line(Point from, Point to, Brush *brush) ABSTRACT;

		// Draw a rectangle.
		virtual void STORM_FN draw(Rect rect, Brush *brush) ABSTRACT;

		// Draw rounded rectangle.
		virtual void STORM_FN draw(Rect rect, Size edges, Brush *brush) ABSTRACT;

		// Draw an oval.
		virtual void STORM_FN oval(Rect rect, Brush *brush) ABSTRACT;

		// Draw a path.
		virtual void STORM_FN draw(Path *path, Brush *brush) ABSTRACT;

		// Fill a rectangle.
		virtual void STORM_FN fill(Rect rect, Brush *brush) ABSTRACT;

		// Fill rounded rectangle.
		virtual void STORM_FN fill(Rect rect, Size edges, Brush *brush) ABSTRACT;

		// Fill the entire area.
		virtual void STORM_FN fill(Brush *brush) ABSTRACT;

		// Fill a path.
		virtual void STORM_FN fill(Path *path, Brush *brush) ABSTRACT;

		// Fill an oval.
		virtual void STORM_FN fillOval(Rect rect, Brush *brush) ABSTRACT;

		// Draw a bitmap.
		void STORM_FN draw(Bitmap *bitmap);
		void STORM_FN draw(Bitmap *bitmap, Point topLeft);
		void STORM_FN draw(Bitmap *bitmap, Point topLeft, Float opacity);
		void STORM_FN draw(Bitmap *bitmap, Rect rect);
		virtual void STORM_FN draw(Bitmap *bitmap, Rect rect, Float opacity) ABSTRACT;

		// Draw a part of a bitmap.
		void STORM_FN draw(Bitmap *bitmap, Rect src, Point topLeft);
		void STORM_FN draw(Bitmap *bitmap, Rect src, Point topLeft, Float opacity);
		void STORM_FN draw(Bitmap *bitmap, Rect src, Rect dest);
		virtual void STORM_FN draw(Bitmap *bitmap, Rect src, Rect dest, Float opacity) ABSTRACT;

		// Draw text the easy way. Prefer using a Text object if possible, since they give more
		// control over the text and yeild better performance. Attempts to fit the given text inside
		// 'rect'. No attempt is made at clipping anything not fitting inside 'rect', so any text
		// not fitting inside 'rect' will be drawn outside the rectangle.
		virtual void STORM_FN text(Str *text, Font *font, Brush *brush, Rect rect) ABSTRACT;

		// Draw pre-formatted text.
		virtual void STORM_FN draw(Text *text, Brush *brush, Point origin) ABSTRACT;


		/**
		 * Create resources specific to this class. Called internally by subclasses to Resource, not
		 * to be used outside of that context.
		 */
		virtual GraphicsResource *STORM_FN create(SolidBrush *brush);
		virtual GraphicsResource *STORM_FN create(LinearGradient *brush);
		virtual GraphicsResource *STORM_FN create(RadialGradient *brush);

	protected:
		// Our identifier. Initialized to 0 (meaning, we don't need resources), but set by some
		// subclasses during creation.
		Nat identifier;

	private:
		// Resources associated with this graphics object. I.e. instances that have created
		// something that is tied to this Graphics instance.
		WeakSet<Resource> *resources;
	};


}
