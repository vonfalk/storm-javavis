#include "stdafx.h"
#include "Rect.h"
#include "Str.h"

namespace storm {
	namespace geometry {

		Rect::Rect() : p0(), p1() {}

		Rect::Rect(Point p, Size s) : p0(p), p1(p + s) {}

		Rect::Rect(Point topLeft, Point bottomRight) : p0(topLeft), p1(bottomRight) {}

		Rect::Rect(Int left, Int top, Int right, Int bottom) : p0(left, top), p1(right, bottom) {}

		wostream &operator <<(wostream &to, Rect r) {
			return to << r.p0 << L"-" << r.p1;
		}

		Str *toS(EnginePtr e, Rect r) {
			return CREATE(Str, e.v, ::toS(r));
		}

	}
}
