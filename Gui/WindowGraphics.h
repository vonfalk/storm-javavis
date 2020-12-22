#pragma once
#include "Graphics.h"

namespace gui {

	/**
	 * Graphics object used for drawing inside a Painter. A suitable subclass to this abstract class
	 * will be created when the Painter is initialized depending on which graphics backend is used.
	 *
	 * This class extends the public interface of Graphics with some internal functions used to
	 * communicate between the Painter and the Graphic class.
	 *
	 * Not usable outside the 'render' function in a Painter.
	 */
	class WindowGraphics : public Graphics {
		STORM_CLASS;
	public:
		// Create.
		WindowGraphics();

		// Called when the associated Surface was resized.
		virtual void surfaceResized();

		// Called when the associated Surface is about to be destroyed.
		virtual void surfaceDestroyed();

		// Prepare for rendering. Given the BG color of the painter.
		virtual void beforeRender(Color bgColor);

		// Housekeeping after rendering. Returns "false" if the surface needs to be re-created for some reason.
		virtual bool afterRender();
	};


	// Define all members of the WindowGraphics as dummies that don't do anything. This is useful
	// when writing a class that only works on some systems, so that a dummy implementation can be
	// generated on other systems. The Storm preprocessor does not support "ignoring" classes in
	// certain conditions as it is not aware of all defines etc. so this is the option we have.
#define DEFINE_WINDOW_GRAPHICS_FNS(CLASS)					\
	void CLASS::surfaceResized() {}							\
	void CLASS::surfaceDestroyed() {}						\
	void CLASS::beforeRender(Color) {}						\
	bool CLASS::afterRender() { return true; }				\
	void CLASS::reset() {}									\
	void CLASS::push() {}									\
	void CLASS::push(Float) {}								\
	void CLASS::push(Rect) {}								\
	void CLASS::push(Rect, Float) {}						\
	Bool CLASS::pop() { return false; }						\
	void CLASS::transform(Transform *) {}					\
	void CLASS::lineWidth(Float) {}							\
	void CLASS::line(Point, Point, Brush *) {}				\
	void CLASS::draw(Rect, Brush *) {}						\
	void CLASS::draw(Rect, Size, Brush *) {}				\
	void CLASS::oval(Rect, Brush *) {}						\
	void CLASS::draw(Path *, Brush *) {}					\
	void CLASS::fill(Rect, Brush *) {}						\
	void CLASS::fill(Rect, Size, Brush *) {}				\
	void CLASS::fill(Brush *) {}							\
	void CLASS::fillOval(Rect, Brush *) {}					\
	void CLASS::fill(Path *, Brush *) {}					\
	void CLASS::draw(Bitmap *, Rect, Float) {}				\
	void CLASS::draw(Bitmap *, Rect, Rect, Float) {}		\
	void CLASS::text(Str *, Font *, Brush *, Rect) {}		\
	void CLASS::draw(Text *, Brush *, Point) {}

}
