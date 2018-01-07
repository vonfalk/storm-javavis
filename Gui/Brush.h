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
			return b;
		}

		// Prepare for drawing a bounding box of 'bound'.
		virtual void prepare(const Rect &bound, ID2D1Brush *b);
#endif
#ifdef GUI_GTK
		// Set the source of the cairo_t to this brush.
		inline void setSource(Painter *owner, cairo_t *c, const Rect &bound) {
			cairo_pattern_t *b = get<cairo_pattern_t>(owner);

			if (b) {
				prepare(bound, b);
				cairo_set_source(c, b);
			} else {
				prepare(bound, c);
			}
		}

		// Prepare for drawing a bounding box of 'bound'.
		virtual void prepare(const Rect &bound, cairo_pattern_t *brush);
		virtual void prepare(const Rect &bound, cairo_t *cairo);
#endif
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

		virtual void prepare(const Rect &bound, ID2D1Brush *b);
#endif
#ifdef GUI_GTK
		virtual OsResource *create(Painter *owner);

		virtual void prepare(const Rect &bound, cairo_t *cairo);
#endif

		// Opacity.
		Float opacity;

	private:
		Color color;
	};

	/**
	 * Bitmap brush.
	 */
	class BitmapBrush : public Brush {
		STORM_CLASS;
	public:
		STORM_CTOR BitmapBrush(Bitmap *bitmap);

#ifdef GUI_WIN32
		virtual void create(Painter *owner, ID2D1Resource **out);

		virtual void prepare(const Rect &bound, ID2D1Brush *b);
#endif
#ifdef GUI_GTK
		virtual OsResource *create(Painter *owner);

		virtual void prepare(const Rect &bound, cairo_pattern_t *brush);
#endif
	private:
		Bitmap *bitmap;
	};

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

	protected:
#ifdef GUI_WIN32
		// Get the stops object.
		ID2D1GradientStopCollection *dxStops(Painter *owner);
#endif
#ifdef GUI_GTK
		// Apply stops to a pattern.
		void applyStops(cairo_pattern_t *to);
#endif
	private:
		ID2D1GradientStopCollection *dxObject;
		Array<GradientStop> *myStops;
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
		// Create.
		virtual OsResource *create(Painter *owner);

		// Prepare.
		virtual void prepare(const Rect &s, cairo_pattern_t *b);
#endif

		// The angle.
		Angle angle;

	private:
		// Compute points.
		void compute(const Rect &sz, Point &start, Point &end);
	};

	// TODO: Implement a radial gradient as well!

}
