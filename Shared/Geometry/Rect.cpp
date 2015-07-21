#include "stdafx.h"
#include "Rect.h"
#include "Str.h"

namespace storm {
	namespace geometry {

		Rect::Rect() : p0(), p1() {}

		Rect::Rect(Point p, Size s) : p0(p), p1(p + s) {}

		Rect::Rect(Point topLeft, Point bottomRight) : p0(topLeft), p1(bottomRight) {}

		Rect::Rect(Float left, Float top, Float right, Float bottom) : p0(left, top), p1(right, bottom) {}

		Rect Rect::normalized() const {
			return Rect(
				Point(min(p0.x, p1.x), min(p0.y, p1.y)),
				Point(max(p0.x, p1.x), max(p0.y, p1.y))
				);
		}

		Rect Rect::operator +(Point pt) const {
			return Rect(p0 + pt, p1 + pt);
		}

		Rect Rect::operator -(Point pt) const {
			return Rect(p0 - pt, p1 - pt);
		}

		Rect &Rect::operator +=(Point pt) {
			p0 += pt;
			p1 += pt;
			return *this;
		}

		Rect &Rect::operator -=(Point pt) {
			p0 -= pt;
			p1 -= pt;
			return *this;
		}

		Rect Rect::include(Point to) const {
			Float l = min(p0.x, to.x);
			Float t = min(p0.y, to.y);
			Float r = max(p1.x, to.x);
			Float b = max(p1.y, to.y);
			return Rect(l, t, r, b);
		}

		wostream &operator <<(wostream &to, Rect r) {
			return to << r.p0 << L"-" << r.p1;
		}

		Str *toS(EnginePtr e, Rect r) {
			return CREATE(Str, e.v, ::toS(r));
		}

	}
}
