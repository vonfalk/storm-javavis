#pragma once
#include "Shared/EnginePtr.h"
#include "Point.h"

namespace storm {
	class Str;
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
			STORM_VAR Float x;
			STORM_VAR Float y;
			STORM_VAR Float z;
		};

		wostream &operator <<(wostream &to, const Vector &s);
		Str *STORM_ENGINE_FN toS(EnginePtr e, Vector s);

		// Operations.
		Vector STORM_FN operator +(Vector a, Vector b);
		Vector STORM_FN operator -(Vector a, Vector b);

		// Scaling.
		Vector STORM_FN operator *(Vector a, Float b);
		Vector STORM_FN operator *(Float a, Vector b);
		Vector STORM_FN operator /(Vector a, Float b);

		// Dot and cross product.
		Vector STORM_FN operator *(Vector a, Vector b);
		Vector STORM_FN operator /(Vector a, Vector b);

		inline Bool STORM_FN operator ==(Vector a, Vector b) { return a.x == b.x && a.y == b.y && a.z == b.z; }
		inline Bool STORM_FN operator !=(Vector a, Vector b) { return !(a == b); }

		Vector STORM_FN abs(Vector a);

	}
}
