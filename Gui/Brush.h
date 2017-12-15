#pragma once
#include "RenderResource.h"
#include "Core/Array.h"

namespace gui {
	class Bitmap;

	/**
	 * Brush (abstract).
	 */
	class Brush : public RenderResource {
		STORM_CLASS;
	public:
		Brush();

#ifdef GUI_WIN32
		// Get a brush.
		inline ID2D1Brush *brush(Painter *owner, const Rect &rect) {
			ID2D1Brush *b = get<ID2D1Brush>(owner);
			prepare(rect, b);
			b->SetOpacity(opacity);
			return b;
		}

		// Prepare for drawing a bounding box of 'bound'.
		virtual void prepare(const Rect &bound, ID2D1Brush *b);
#endif
#ifdef GUI_GTK
		// Stroke the current path.
		void stroke(NVGcontext *c, const Rect &bound, Float opacity);

		// Fill the current path.
		void fill(NVGcontext *c, const Rect &bound, Float opacity);

		// Set the stroke of the NVGcontext.
		virtual void setStroke(NVGcontext *c, const Rect &bound);

		// Set the fill of the NVGcontext.
		virtual void setFill(NVGcontext *c, const Rect &bound);
#endif

		// Opacity.
		Float opacity;
	};

	/**
	 * Solid brush.
	 */
	class SolidBrush : public Brush {
		STORM_CLASS;
	public:
		STORM_CTOR SolidBrush(Color color);

#ifdef GUI_WIN32
		virtual void create(Painter *owner, ID2D1Resource **out);
#endif
#ifdef GUI_GTK
		virtual void setStroke(NVGcontext *c, const Rect &bound);
		virtual void setFill(NVGcontext *c, const Rect &bound);
#endif

	private:
		Color color;
	};

	/**
	 * Bitmap brush.
	 *
	 * TODO: Implement properly on both platforms.
	 */
	// class BitmapBrush : public Brush {
	// 	STORM_CLASS;
	// public:
	// 	STORM_CTOR BitmapBrush(Bitmap *bitmap);

	// 	virtual void create(Painter *owner, ID2D1Resource **out);

	// 	virtual void prepare(const Rect &bound, ID2D1Brush *b);
	// private:
	// 	Bitmap *bitmap;
	// };

	/**
	 * A single gradient stop.
	 */
	class GradientStop {
		STORM_VALUE;
	public:
		STORM_CTOR GradientStop(Float position, Color color);

		Float pos;
		Color color;
	};

	wostream &operator <<(wostream &to, const GradientStop &s);
	StrBuf &STORM_FN operator <<(StrBuf &to, GradientStop s);

	/**
	 * Gradient of some kind.
	 */
	class Gradient : public Brush {
		STORM_CLASS;
	public:
		Gradient();
		Gradient(Array<GradientStop> *stops);
		~Gradient();

		// Set the stops.
		void STORM_SETTER stops(Array<GradientStop> *stops);

		// Destroy the stops.
		virtual void destroy();

#ifdef GUI_GTK
		virtual void setStroke(NVGcontext *c, const Rect &bound);
		virtual void setFill(NVGcontext *c, const Rect &bound);

		// Called to create the paint for this gradient.
		virtual NVGpaint createPaint(NVGcontext *c, const Rect &bound);
#endif
	protected:
#ifdef GUI_WIN32
		// Get the stops object.
		ID2D1GradientStopCollection *dxStops(Painter *owner);
#endif
		// Current stops.
		Array<GradientStop> *myStops;

	private:
		// DX object containing the gradient stops.
		ID2D1GradientStopCollection *dxObject;
	};


	/**
	 * Linear gradient.
	 */
	class LinearGradient : public Gradient {
		STORM_CLASS;
	public:
		STORM_CTOR LinearGradient(Array<GradientStop> *stops, Angle angle);

		// Create two stops at 0 and 1.
		STORM_CTOR LinearGradient(Color c1, Color c2, Angle angle);

#ifdef GUI_WIN32
		// Get the brush.
		inline ID2D1LinearGradientBrush *brush(Painter *owner) { return get<ID2D1LinearGradientBrush>(owner); }

		// Create.
		virtual void create(Painter *owner, ID2D1Resource **out);

		// Prepare.
		virtual void prepare(const Rect &s, ID2D1Brush *b);
#endif
#ifdef GUI_GTK
		// Create the NVGpaint to use for this gradient.
		virtual NVGpaint createPaint(NVGcontext *c, const Rect &bound);

		// Destroy any resources here.
		virtual void destroy();
#endif

		// The angle.
		Angle angle;

	private:
		// Compute points.
		void compute(const Rect &sz, Point &start, Point &end);
		void compute(const Rect &sz, Point &start, Point &end, Float &distance);
	};

	// TODO: Implement a radial gradient as well!

}
