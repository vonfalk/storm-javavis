#pragma once
#include "Core/EnginePtr.h"
#include "Point.h"

namespace storm {
	class Str;
	class StrBuf;
	namespace geometry {
		STORM_PKG(core.geometry);

		/**
		 * A point in 3D-space.
		 */
		class Vector {
			STORM_VALUE;
		public:
			// Zero.
			STORM_CTOR Vector();

			// Initialize.
			STORM_CTOR Vector(Float x, Float y, Float z);
			STORM_CAST_CTOR Vector(Point p);

			// Coordinates.
			Float x;
			Float y;
			Float z;
		};

		wostream &operator <<(wostream &to, const Vector &s);
		StrBuf &operator <<(StrBuf &to, Vector s);

		// Operations.
		Vector STORM_FN operator +(Vector a, Vector b);
		Vector STORM_FN operator -(Vector a, Vector b);

		// Scaling.
		Vector STORM_FN operator *(Vector a, Float b);
		Vector STORM_FN operator *(Float a, Vector b);
		Vector STORM_FN operator /(Vector a, Float b);

		// Dot and cross product.
		Float STORM_FN operator *(Vector a, Vector b);
		Vector STORM_FN operator /(Vector a, Vector b);

		inline Bool STORM_FN operator ==(Vector a, Vector b) { return a.x == b.x && a.y == b.y && a.z == b.z; }
		inline Bool STORM_FN operator !=(Vector a, Vector b) { return !(a == b); }

		Vector STORM_FN abs(Vector a);

	}
}
