#include "stdafx.h"
#include "Angle.h"
#include "Str.h"

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

		Angle STORM_FN operator *(Angle a, Float b) {
			return Angle(a.rad() * b);
		}

		Angle STORM_FN operator *(Float a, Angle b) {
			return Angle(a * b.rad());
		}

	}
}
