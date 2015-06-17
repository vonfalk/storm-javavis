#pragma once
#include "Size.h"

namespace storm {
	namespace geometry {

		/**
		 * A point in 2D-space.
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
		};

		wostream &operator <<(wostream &to, const Point &s);
		Str *toS(EnginePtr e, Point s);

		// Operations.
		Point STORM_FN operator +(Point a, Size b);
		Point STORM_FN operator +(Point a, Point b);
		Size STORM_FN operator -(Point a, Point b);
		Point STORM_FN operator -(Point a, Size b);

		inline Bool STORM_FN operator ==(Point a, Point b) { return a.x == b.x && a.y == b.y; }
		inline Bool STORM_FN operator !=(Point a, Point b) { return !(a == b); }

	}
}
