#pragma once
#include "Shared/Object.h"
#include "Shared/EnginePtr.h"
#include "Vector.h"

namespace storm {
	namespace geometry {
		STORM_PKG(core.geometry);

		/**
		 * Transform in 2 or 3 dimensions. 2d-coordinates are assumed to lie in the xy-plane.
		 */
		class Transform : public Object {
			STORM_CLASS;
		public:
			// Unit transform.
			STORM_CTOR Transform();

			// Copy.
			STORM_CTOR Transform(Par<Transform> o);

			// Create and initialize. Row by row.
			Transform(Float data[16]);

			// Operations...
			Transform *STORM_FN operator *(Par<Transform> o);
			Vector STORM_FN operator *(Vector o);
			Point STORM_FN operator *(Point o);

			// Invert.
			Transform *STORM_FN inverted();

		private:
			Transform(Float data[4][4]);

			// Data (y, x) (row, col)
 			Float v[4][4];
		};

		// Translation transform.
		Transform *STORM_ENGINE_FN translate(EnginePtr e, Vector v);
		Transform *STORM_ENGINE_FN rotateX(EnginePtr e, Float angle);
		Transform *STORM_ENGINE_FN rotateY(EnginePtr e, Float angle);
		Transform *STORM_ENGINE_FN rotateZ(EnginePtr e, Float angle);
		Transform *STORM_ENGINE_FN rotate(EnginePtr e, Float angle); // Same as rotateZ
		Transform *STORM_ENGINE_FN scale(EnginePtr e, Float scale);
		Transform *STORM_ENGINE_FN scale(EnginePtr e, Vector scale);

	}
}
