#pragma once

namespace storm {
	class Str;
	namespace geometry {
		class Point;
		STORM_PKG(core.geometry);

		/**
		 * Angle in radians. 0 deg is assumed to be upward in screen-space (ie 0, -1)
		 * TODO: Some kind of comparisons for angles.
		 */
		class Angle {
			STORM_VALUE;
		public:
			// Zero angle.
			inline STORM_CTOR Angle() { v = 0; }

			// Angle in radians (intentionally not visible to Storm).
			inline Angle(Float rad) { v = rad; }

			// Normalized.
			Angle STORM_FN normalized() const;

			// Opposite direction.
			Angle STORM_FN opposite() const;

			// Convert to radians.
			inline Float STORM_FN rad() const { return v; }

			// Convert to degrees.
			inline Float STORM_FN deg() const { return float(v * 180.0 / M_PI); }

		private:
			// Value (intentionally not visible to Storm).
			Float v;
		};

		// Add/subtract.
		Angle STORM_FN operator +(Angle a, Angle b);
		Angle STORM_FN operator -(Angle a, Angle b);
		Angle STORM_FN operator -(Angle a);

		// Scale.
		Angle STORM_FN operator *(Angle a, Float b);
		Angle STORM_FN operator *(Float a, Angle b);
		Angle STORM_FN operator /(Angle a, Float b);


		// To String.
		wostream &operator <<(wostream &to, Angle a);
		StrBuf &STORM_FN operator <<(StrBuf &to, Angle a);

		// Create an angle.
		Angle STORM_FN deg(Float v);
		Angle STORM_FN rad(Float v);

		// Sin and cos of angles.
		Float STORM_FN sin(Angle v);
		Float STORM_FN cos(Angle v);
		Float STORM_FN tan(Angle v);

		// Asin, acos.
		Angle STORM_FN asin(Float v);
		Angle STORM_FN acos(Float v);
		Angle STORM_FN atan(Float v);
		Angle STORM_FN angle(Point pt);

		// Get pi.
		inline Float STORM_FN pi() { return Float(M_PI); }
	}
}
