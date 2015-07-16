#pragma once
#include "RenderResource.h"

namespace stormgui {

	/**
	 * A point-by-point path to draw.
	 *
	 * Paths can be either open or closed when they are drawn as an outline. When filling, it is
	 * assumed that all paths are closed with a line from the last point to the first point.
	 */
	class Path : public ObjectOn<Render> {
		STORM_CLASS;
	public:
		// Create.
		STORM_CTOR Path();

		// Destroy.
		~Path();

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

		// Get the geometry object.
		ID2D1PathGeometry *geometry();

	private:
		// Element type.
		enum Type {
			tClose,
			tStart,
			tLine,
			tBezier2,
			tBezier3,
		};

		typedef D2D1_POINT_2F Pt;

		// Different element types.
		struct Start {
			Pt pt;
		};

		struct Line {
			Pt to;
		};

		struct Bezier2 {
			Pt c1;
			Pt to;
		};

		struct Bezier3 {
			Pt c1;
			Pt c2;
			Pt to;
		};

		// Element.
		struct Element {
			Type t;
			union {
				Start start;
				Line line;
				Bezier2 bezier2;
				Bezier3 bezier3;
			};
		};

		// All elements.
		vector<Element> elements;

		// Any path started?
		bool started;

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
