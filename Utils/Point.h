#pragma once

#include "Size.h"
#include "Vector.h"
#include <Gdiplus.h>

// A point in 2D space. Used in HUD:s and UI:s.
class Point {
public:
	int x, y;

	inline Point() : x(0), y(0) {}
	inline Point(int x, int y) : x(x), y(y) {}
	inline Point(Size sz) : x(sz.w), y(sz.h) {}
	inline Point(POINT pt) : x(pt.x), y(pt.y) {}

	inline operator Gdiplus::Point() const { return Gdiplus::Point(x, y); }

	inline POINT win() const {
		POINT pt = { x, y };
		return pt;
	}

	inline Point operator +(const Point &o) const { return Point(x + o.x, y + o.y); }
	inline Point operator +(const Size &o) const { return Point(x + o.w, y + o.h); }
	inline Size operator -(const Point &o) const { return Size(x - o.x, y - o.y); }
	inline Point operator -(const Size &o) const { return Point(x - o.w, y - o.h); }
	inline Point operator -() const { return Point(-x, -y); }
	inline Point operator *(int i) const { return Point(x * i, y * i); }
	inline Point operator /(int i) const { return Point(x / i, y / i); }

	inline Point &operator +=(const Point &o) { x += o.x; y += o.y; return *this; }
	inline Point &operator -=(const Point &o) { x -= o.x; y -= o.y; return *this; }
	inline Point &operator +=(const Size &o) { x += o.w; y += o.h; return *this; }
	inline Point &operator -=(const Size &o) { x -= o.w; y -= o.h; return *this; }
	inline Point &operator *=(int i) { x *= i; y *= i; return *this; }
	inline Point &operator /=(int i) { x /= i; y /= i; return *this; }

	inline Vector to3D() const { return Vector(float(x), float(y), 0.5f); }

	inline bool operator ==(const Point &o) const { return x == o.x && y == o.y; }
	inline bool operator !=(const Point &o) const { return x != o.x || y != o.y; }

	// Get the length of this point.
	float length() const { return sqrt(float(x * x + y * y)); }
};

inline std::wostream &operator <<(std::wostream &to, const Point &pt) {
	to << "(" << pt.x << ", " << pt.y << ")";
	return to;
}