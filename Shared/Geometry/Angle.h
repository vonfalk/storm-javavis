#pragma once
#include "Shared/EnginePtr.h"

namespace storm {
	class Str;
	namespace geometry {
		STORM_PKG(core.geometry);

		/**
		 * Angle in radians.
		 * TODO: Some kind of comparisons for angles.
		 */
		class Angle {
			STORM_VALUE;
		public:
			// Zero angle.
			inline STORM_CTOR Angle() : v(0) {}

			// Angle in radians (intentionally not visible to Storm).
			inline Angle(Float rad) : v(rad) {}

			// Convert to radians.
			inline Float STORM_FN rad() const { return v; }

			// Convert to degrees.
			inline Float STORM_FN deg() const { return float(v * 180.0 / M_PI); }

		private:
			// Value (intentionally not visible to Storm).
			Float v;
		};

		// Scale.
		Angle STORM_FN operator *(Angle a, Float b);
		Angle STORM_FN operator *(Float a, Angle b);


		// To String.
		wostream &operator <<(wostream &to, Angle a);
		Str *STORM_ENGINE_FN toS(EnginePtr e, Angle a);

		// Create an angle.
		Angle STORM_FN deg(Float v);
		Angle STORM_FN rad(Float v);

	}
}
