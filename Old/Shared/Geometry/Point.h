#pragma once
#include "Size.h"
#include "Angle.h"

namespace storm {
	namespace geometry {
		STORM_PKG(core.geometry);

		/**
		 * A point in 2D-space.
		 * Screen-space is assumed, where 0,0 is positioned in the upper left corner of the screen.
		 */
		class Point {
			STORM_VALUE;
		public:
			// Zero.
			STORM_CTOR Point();

			// Initialize.
			STORM_CTOR Point(Float x, Float y);

			// Convert.
			explicit STORM_CTOR Point(Size s);

			// Coordinates.
			STORM_VAR Float x;
			STORM_VAR Float y;

			// Tangent. Always 90 deg cw from the vector in screen-space.
			Point STORM_FN tangent() const;

			// Length squared.
			Float STORM_FN lengthSq() const;

			// Length.
			Float STORM_FN length() const;
		};

		wostream &operator <<(wostream &to, const Point &s);
		Str *STORM_ENGINE_FN toS(EnginePtr e, Point s);

		// Operations.
		Point STORM_FN operator +(Point a, Size b);
		Point STORM_FN operator +(Point a, Point b);
		Point STORM_FN operator -(Point a, Point b);
		Point STORM_FN operator -(Point a, Size b);
		Point &STORM_FN operator +=(Point &a, Point b);
		Point &STORM_FN operator -=(Point &a, Point b);

		Point STORM_FN operator *(Point a, Float b);
		Point STORM_FN operator *(Float a, Point b);
		Point STORM_FN operator /(Point a, Float b);
		Point &STORM_FN operator *=(Point &a, Float b);
		Point &STORM_FN operator /=(Point &a, Float b);

		// Dot product.
		Float STORM_FN operator *(Point a, Point b);

		inline Bool STORM_FN operator ==(Point a, Point b) { return a.x == b.x && a.y == b.y; }
		inline Bool STORM_FN operator !=(Point a, Point b) { return !(a == b); }

		// Create an angle based on the point. 0 deg is (0, -1)
		Point STORM_FN angle(Angle angle);

		// Project a point onto a line. 'dir' does not have to be normalized.
		Point STORM_FN project(Point pt, Point origin, Point dir);

		// Center point of a size.
		Point STORM_FN center(Size s);

		Point STORM_FN abs(Point a);

	}
}
