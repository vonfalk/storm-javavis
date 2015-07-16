#include "stdafx.h"
#include "Angle.h"
#include "Str.h"
#include "Point.h"

namespace storm {
	namespace geometry {

		wostream &operator <<(wostream &to, Angle a) {
			return to << a.deg() << L" deg";
		}

		Str *toS(EnginePtr e, Angle a) {
			return CREATE(Str, e.v, ::toS(a));
		}

		Angle deg(Float v) {
			return Angle(float(v * M_PI / 180.0));
		}

		Angle rad(Float v) {
			return Angle(v);
		}

		Angle operator *(Angle a, Float b) {
			return Angle(a.rad() * b);
		}

		Angle operator *(Float a, Angle b) {
			return Angle(a * b.rad());
		}

		Angle operator /(Angle a, Float b) {
			return Angle(a.rad() / b);
		}

		Float sin(Angle v) {
			return ::sin(v.rad());
		}

		Float cos(Angle v) {
			return ::cos(v.rad());
		}

		Float tan(Angle v) {
			return ::tan(v.rad());
		}

		Angle asin(Float v) {
			return rad(::asin(v));
		}

		Angle acos(Float v) {
			return rad(::acos(v));
		}

		Angle atan(Float v) {
			return rad(::atan(v));
		}

		Angle atan(Point pt) {
			return rad(::atan2(pt.y, pt.x));
		}

	}
}
