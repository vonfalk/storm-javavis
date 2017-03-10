#pragma once
#include "Core/Object.h"
#include "Core/EnginePtr.h"
#include "Vector.h"
#include "Angle.h"

namespace storm {
	namespace geometry {
		STORM_PKG(core.geometry);

		/**
		 * Transform in 2 or 3 dimensions. 2d-coordinates are assumed to lie in the xy-plane.
		 * Transforms are combined like this:
		 * a * b * c, then c is applied first, then b, then c.
		 */
		class Transform : public Object {
			STORM_CLASS;
		public:
			// Unit transform.
			STORM_CTOR Transform();

			// Copy.
			Transform(const Transform &o);

			// Create and initialize. Row by row.
			typedef Float FData[16];
			Transform(FData data);

			// Operations...
			Transform *STORM_FN operator *(Transform *o);
			Vector STORM_FN operator *(Vector o);
			Point STORM_FN operator *(Point o);

			// Invert.
			Transform *STORM_FN inverted();

			// Get elements.
			inline Float at(Nat row, Nat col) const { return (&v00)[row + 4*col]; }

			// To string.
			virtual void STORM_FN toS(StrBuf *to) const;

		private:
			typedef Float FArray[4][4];
			Transform(FArray data);

			inline Float &v(Nat row, Nat col) {
				return (&v00)[row + 4*col];
			}

			// Data (y, x) (row, col). In memory, this is row-by-row.
			Float v00;
			Float v10;
			Float v20;
			Float v30;
			Float v01;
			Float v11;
			Float v21;
			Float v31;
			Float v02;
			Float v12;
			Float v22;
			Float v32;
			Float v03;
			Float v13;
			Float v23;
			Float v33;
		};

		// Translation transform.
		Transform *STORM_FN translate(EnginePtr e, Vector v);
		Transform *STORM_FN translate(EnginePtr e, Point v);
		Transform *STORM_FN translate(EnginePtr e, Size v);
		Transform *STORM_FN rotateX(EnginePtr e, Angle angle);
		Transform *STORM_FN rotateX(EnginePtr e, Angle angle, Vector origin);
		Transform *STORM_FN rotateY(EnginePtr e, Angle angle);
		Transform *STORM_FN rotateY(EnginePtr e, Angle angle, Vector origin);
		Transform *STORM_FN rotateZ(EnginePtr e, Angle angle);
		Transform *STORM_FN rotateZ(EnginePtr e, Angle angle, Vector origin);
		Transform *STORM_FN rotate(EnginePtr e, Angle angle); // Same as rotateZ
		Transform *STORM_FN rotate(EnginePtr e, Angle angle, Point origin);
		Transform *STORM_FN scale(EnginePtr e, Float scale);
		Transform *STORM_FN scale(EnginePtr e, Vector scale);
		Transform *STORM_FN scale(EnginePtr e, Float scale, Vector origin);
		Transform *STORM_FN skewX(EnginePtr e, Angle angle);
		Transform *STORM_FN skewY(EnginePtr e, Angle angle);
		Transform *STORM_FN skewZ(EnginePtr e, Angle angle);

	}
}
