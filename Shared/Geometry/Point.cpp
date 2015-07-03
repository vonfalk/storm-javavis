#include "stdafx.h"
#include "Point.h"
#include "Str.h"

namespace storm {
	namespace geometry {

		Point::Point() : x(0), y(0) {}

		Point::Point(Float x, Float y) : x(x), y(y) {}

		Point::Point(Size s) : x(s.w), y(s.h) {}

		Point Point::tangent() const {
			return Point(-y, x);
		}

		Float Point::lengthSq() const {
			return *this * *this;
		}

		Float Point::length() const {
			return sqrt(lengthSq());
		}

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

		Point operator -(Point a, Point b) {
			return Point(a.x - b.x, a.y - b.y);
		}

		Point operator -(Point a, Size b) {
			return Point(a.x - b.w, a.y - b.h);
		}

		Point &STORM_FN operator +=(Point &a, Point b) {
			a.x += b.x;
			a.y += b.y;
			return a;
		}

		Point &STORM_FN operator -=(Point &a, Point b) {
			a.x -= b.x;
			a.y -= b.y;
			return a;
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

		Point &STORM_FN operator *=(Point &a, Float b) {
			a.x *= b;
			a.y *= b;
			return a;
		}

		Point &STORM_FN operator /=(Point &a, Float b) {
			a.x /= b;
			a.y /= b;
			return a;
		}

		Float operator *(Point a, Point b) {
			return a.x*b.x + a.y*b.y;
		}

		Point abs(Point a) {
			return Point(::abs(a.x), ::abs(a.y));
		}

		Point angle(Angle a) {
			return Point(sin(a.rad()), -cos(a.rad()));
		}

		Point center(Size s) {
			return Point(s.w / 2, s.h / 2);
		}

		Point project(Point pt, Point origin, Point dir) {
			pt -= origin;
			float t = (pt * dir) / (dir * dir);
			return origin + dir * t;
		}

	}
}
