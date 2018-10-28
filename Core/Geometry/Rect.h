#pragma once
#include "Size.h"
#include "Point.h"

namespace storm {
	namespace geometry {
		STORM_PKG(core.geometry);

		/**
		 * 2D rectangle represented by two points p0 and p1 (top left, bottom right).
		 */
		class Rect {
			STORM_VALUE;
		public:
			// Empty rectangle.
			STORM_CTOR Rect();

			// Initialize to (0, 0)-(size)
			STORM_CTOR Rect(Size s);

			// Initialize to point and size.
			STORM_CTOR Rect(Point p, Size s);

			// Initialize to two points.
			STORM_CTOR Rect(Point topLeft, Point bottomRight);

			// Initialize to raw coordinates.
			STORM_CTOR Rect(Float left, Float top, Float right, Float bottom);

			// Data.
			Point p0;
			Point p1;

			// Normalized.
			Rect STORM_FN normalized() const;

			// Contains a point?
			Bool STORM_FN contains(Point pt) const;

			// Size.
			inline Size STORM_FN size() const { return p1 - p0; }
			inline void STORM_ASSIGN size(Size to) { p1 = p0 + to; }
			inline Rect STORM_FN sized(Size to) { Rect r = *this; r.size(to); return r; }

			// Center.
			inline Point STORM_FN center() const { return (p0 + p1) / 2; }

			// Move.
			inline Rect STORM_FN at(Point to) const { return Rect(to, size()); }

			Rect STORM_FN operator +(Point pt) const;
			Rect STORM_FN operator -(Point pt) const;
			Rect &STORM_FN operator +=(Point pt);
			Rect &STORM_FN operator -=(Point pt);

			// Include a point in the bounding box.
			Rect STORM_FN include(Point to) const;

			// Include a rectangle. Yields the bounding box of this rectangle and 'other'.
			Rect STORM_FN include(Rect other) const;

			// Scale the entire rect around its center.
			Rect STORM_FN scaled(Float scale) const;

			// Shrink/grow according to a size.
			Rect STORM_FN shrink(Size rate) const;
			Rect STORM_FN grow(Size rate) const;
		};

		// Point inside.
		Bool STORM_FN inside(Point pt, Rect r);

		// ToS.
		wostream &operator <<(wostream &to, Rect r);
		StrBuf &STORM_FN operator <<(StrBuf &to, Rect r);

		inline Bool STORM_FN operator ==(Rect a, Rect b) { return a.p0 == b.p0 && a.p1 == b.p1; }
		inline Bool STORM_FN operator !=(Rect a, Rect b) { return !(a == b); }
	}
}
