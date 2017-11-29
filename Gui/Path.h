#pragma once
#include "Core/Array.h"
#include "Resource.h"

namespace gui {

	/**
	 * A point-by-point path to draw.
	 *
	 * Paths can be either open or closed when they are drawn as an outline. When filling, it is
	 * assumed that all paths are closed with a line from the last point to the first point.
	 */
	class Path : public Resource {
		STORM_CLASS;
	public:
		// Create.
		STORM_CTOR Path();

		// Destroy.
		~Path();

		// Destroy.
		virtual void destroy();

		// Clear this path.
		void STORM_FN clear();

		// Start a new segment.
		void STORM_FN start(Point pt);

		// Add a line from the current point to 'to'.
		void STORM_FN line(Point to);

		// Add a point. If the path is not started, this point will start it. Aside from that, works like 'line'.
		void STORM_FN point(Point to);

		// Add a bezier segment with one control point and an endpoint.
		void STORM_FN bezier(Point c1, Point to);

		// Add a bezier segment with two control points and an endpoint.
		void STORM_FN bezier(Point c1, Point c2, Point to);

		// Close the previous path.
		void STORM_FN close();

		// Get the bounding box of this path.
		inline Rect STORM_FN bound() { return b; }

#ifdef GUI_WIN32
		// Get the geometry object.
		ID2D1PathGeometry *geometry();
#endif
#ifdef GUI_GTK
		// Set the path on the supplied cairo_t.
		void draw(cairo_t *c);
#endif
	private:
		// Element type.
		enum Type {
			tClose,
			tStart,
			tLine,
			tBezier2,
			tBezier3,
		};

		// Different element types.
		struct Start {
			Point pt;
		};

		struct Line {
			Point to;
		};

		struct Bezier2 {
			Point c1;
			Point to;
		};

		struct Bezier3 {
			Point c1;
			Point c2;
			Point to;
		};

		// Element. One of five different types.
		class Element {
			STORM_VALUE;
		public:
			Element(Type type) {
				t = type;
			}

			Type t;
			Point p0;
			Point p1;
			Point p2;

			// Convert to other types for convenient access.
			inline Start *start() {
				return (Start *)&p0;
			}
			inline Line *line() {
				return (Line *)&p0;
			}
			inline Bezier2 *bezier2() {
				return (Bezier2 *)&p0;
			}
			inline Bezier3 *bezier3() {
				return (Bezier3 *)&p0;
			}
		};

		// All elements.
		Array<Element> *elements;

		// Any path started?
		Bool started;

		// Bound.
		Rect b;

		// The underlying DX object.
		ID2D1PathGeometry *g;

		// Invalidate this path.
		void invalidate();

		// Create the path.
		void create();
	};

}
