#include "stdafx.h"
#include "Point.h"
#include "Str.h"

namespace storm {
	namespace geometry {

		Point::Point() : x(0), y(0) {}

		Point::Point(Float x, Float y) : x(x), y(y) {}

		Point::Point(Size s) : x(s.w), y(s.h) {}

		wostream &operator <<(wostream &to, const Point &p) {
			return to << L"(" << p.x << L", " << p.y << L")";
		}

		Str *toS(EnginePtr e, Point p) {
			return CREATE(Str, e.v, ::toS(p));
		}

		Point operator +(Point a, Size b) {
			return Point(a.x + b.w, a.y + b.h);
		}

		Point operator +(Point a, Point b) {
			return Point(a.x + b.x, a.y + b.y);
		}

		Size operator -(Point a, Point b) {
			return Size(a.x - b.x, a.y - b.y);
		}

		Point operator -(Point a, Size b) {
			return Point(a.x - b.w, a.y - b.h);
		}

		Point operator *(Point a, Float b) {
			return Point(a.x * b, a.y * b);
		}

		Point operator *(Float a, Point b) {
			return Point(a * b.x, a * b.y);
		}

		Point operator /(Point a, Float b) {
			return Point(a.x / b, a.y / b);
		}

	}
}
