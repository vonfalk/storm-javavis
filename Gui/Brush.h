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
		inline ID2D1Brush *brush(Painter *owner) {
			ID2D1Brush *b = get<ID2D1Brush>(owner);
			prepare(b);
			return b;
		}

		// Prepare for drawing a bounding box of 'bound'.
		virtual void prepare(ID2D1Brush *b);
#endif
#ifdef GUI_GTK
		// Set the source of the cairo_t to this brush.
		inline void setSource(Painter *owner, cairo_t *c) {
			cairo_pattern_t *b = get<cairo_pattern_t>(owner);
			if (b) {
				cairo_set_source(c, b);
			} else {
				prepare(c);
			}
		}

		// Prepare for drawing, if 'create' returns null.
		virtual void prepare(cairo_t *cairo);
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

		virtual void prepare(ID2D1Brush *b);
#endif
#ifdef GUI_GTK
		virtual OsResource *create(Painter *owner);

		virtual void prepare(cairo_t *cairo);
#endif

		// Opacity.
		Float opacity;

		// Color.
		inline Color STORM_FN color() const { return col; }

	private:
		// Color.
		Color col;
	};

	/**
	 * Bitmap brush.
	 */
	class BitmapBrush : public Brush {
		STORM_CLASS;
	public:
		STORM_CTOR BitmapBrush(Bitmap *bitmap);
		STORM_CTOR BitmapBrush(Bitmap *bitmap, Transform *tfm);

		// Get the bitmap.
		inline Bitmap *STORM_FN bitmap() const { return myBitmap; }

		// Get the current transform.
		inline Transform *STORM_FN transform() const { return myTfm; }

		// Set the current transform.
		void STORM_ASSIGN transform(Transform *tfm);

#ifdef GUI_WIN32
		virtual void create(Painter *owner, ID2D1Resource **out);
#endif
#ifdef GUI_GTK
		virtual OsResource *create(Painter *owner);
#endif
	private:
		// The actual bitmap.
		Bitmap *myBitmap;

		// The transform when applying the brush.
		Transform *myTfm;
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

		// Get the stops.
		Array<GradientStop> *STORM_FN stops() const;

		// Set the stops.
		void STORM_ASSIGN stops(Array<GradientStop> *stops);

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
		// Provide the stops.
		STORM_CTOR LinearGradient(Array<GradientStop> *stops, Point start, Point end);

		// Create two stops at 0 and 1.
		STORM_CTOR LinearGradient(Color c1, Color c2, Point start, Point end);

		// Get start and end points.
		inline Point STORM_FN start() const { return myStart; }
		inline Point STORM_FN end() const { return myEnd; }

		// Set start and end points.
		void STORM_ASSIGN start(Point p);
		void STORM_ASSIGN end(Point p);

		// Set both.
		void STORM_FN points(Point start, Point end);

#ifdef GUI_WIN32
		// Create.
		virtual void create(Painter *owner, ID2D1Resource **out);
#endif
#ifdef GUI_GTK
		// Create.
		virtual OsResource *create(Painter *owner);
#endif

	private:
		// Start and end point of the gradient.
		Point myStart;
		Point myEnd;

		// Update the points in the underlying representation (if any).
		void updatePoints();

#ifdef GUI_GTK
		// Set the transform of 'p'.
		void updatePoints(cairo_pattern_t *p);
#endif
	};


	/**
	 * Radial gradient.
	 */
	class RadialGradient : public Gradient {
		STORM_CLASS;
	public:
		// Provide the stops.
		STORM_CTOR RadialGradient(Array<GradientStop> *stops, Point center, Float radius);

		// Create two stops at 0 and 1.
		STORM_CTOR RadialGradient(Color c1, Color c2, Point center, Float radius);

		// Get/set the center point.
		inline Point STORM_FN center() const { return myCenter; }
		void STORM_ASSIGN center(Point pt);

		// Get/set the radius.
		inline Float STORM_FN radius() const { return myRadius; }
		void STORM_ASSIGN radius(Float radius);

		// Get/set a generic transform.
		Transform *STORM_FN transform() const { return myTransform; }
		void STORM_ASSIGN transform(Transform *tfm);

#ifdef GUI_WIN32
		// Create.
		virtual void create(Painter *owner, ID2D1Resource **out);
#endif
#ifdef GUI_GTK
		// Create.
		virtual OsResource *create(Painter *owner);
#endif

	private:
		// Center point and radius.
		Point myCenter;
		Float myRadius;

		// Transform.
		Transform *myTransform;

		// Update the properties in the underlying representation (if any).
		void update();

#ifdef GUI_GTK
		// Set the transform of 'p'.
		void update(cairo_pattern_t *p);
#endif
	};

}
