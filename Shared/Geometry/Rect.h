#pragma once
#include "Size.h"
#include "Point.h"

namespace storm {
	namespace geometry {

		/**
		 * 2D rectangle represented by two points p0 and p1 (top left, bottom right).
		 */
		class Rect {
			STORM_VALUE;
		public:
			// Empty rectangle.
			STORM_CTOR Rect();

			// Initialize to point and size.
			STORM_CTOR Rect(Point p, Size s);

			// Initialize to two points.
			STORM_CTOR Rect(Point topLeft, Point bottomRight);

			// Initialize to raw coordinates.
			STORM_CTOR Rect(Int left, Int top, Int right, Int bottom);

			// Data.
			STORM_VAR Point p0;
			STORM_VAR Point p1;

			// Size.
			inline Size STORM_FN size() const { return p1 - p0; }
			inline void STORM_SETTER size(Size to) { p1 = p0 + to; }
			inline Rect STORM_FN sized(Size to) { Rect r = *this; r.size(to); return r; }

			// Center.
			inline Point STORM_FN center() const { return Point((p0.x + p1.x) / 2, (p0.y + p1.y) / 2); }

			// Move.
			inline Rect STORM_FN at(Point to) const { return Rect(to, size()); }
		};

		// ToS.
		wostream &operator <<(wostream &to, Rect r);
		Str *STORM_ENGINE_FN toS(EnginePtr e, Rect r);

		inline Bool STORM_FN operator ==(Rect a, Rect b) { return a.p0 == b.p0 && a.p1 == b.p1; }
		inline Bool STORM_FN operator !=(Rect a, Rect b) { return !(a == b); }
	}
}
