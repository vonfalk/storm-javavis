#pragma once

#include "Point.h"
#include "Size.h"
#include "Border.h"
#include <Gdiplus.h>

class Rect {
public:
	union {
		struct {
			// Top-left point
			Point p0;
			// Bottom-right point
			Point p1;
		};
		struct {
			int left;
			int top;
			int right;
			int bottom;
		};
	};

	inline Rect() {}
	inline Rect(Point topLeft, Size size) : p0(topLeft), p1(topLeft + size) {}
	inline Rect(Point topLeft, Point bottomRight) : p0(topLeft), p1(bottomRight) {}
	inline Rect(int left, int top, int right, int bottom) : p0(left, top), p1(right, bottom) {}
	inline explicit Rect(Size size) : p0(0, 0), p1(size.w, size.h) {}
	inline Rect(RECT r) : p0(r.left, r.top), p1(r.right, r.bottom) {}
	inline Rect(const Gdiplus::Rect &r) : p0(r.X, r.Y), p1(r.X + r.Width, r.Y + r.Height) {}
	inline Rect(const Gdiplus::RectF &r) : p0(int(r.X), int(r.Y)), p1(int(r.X + r.Width), int(r.Y + r.Height)) {}
	inline operator Gdiplus::Rect() const { return Gdiplus::Rect(p0.x, p0.y, width(), height()); }

	inline RECT win() const { RECT r = { p0.x, p0.y, p1.x, p1.y}; return r; }

	inline Size size() const { return p1 - p0; }
	inline int width() const { return p1.x - p0.x; }
	inline int height() const { return p1.y - p0.y; }

	inline Point center() const {
		return (p0 + p1) / 2;
	}

	inline void setSize(Size sz) {
		p1 = p0 + sz;
	}

	// Make sure p0 is above and to the left of p1
	void normalize();

	// Ensure the rect's top left corner is in the given position.
	inline Rect in(Point &pos) const { return Rect(pos, size()); }

	// Move relative a position.
	inline Rect operator +(const Point &relative) { return Rect(p0 + relative, p1 + relative); }
	Rect &operator +=(const Point &relative) { p0 += relative; p1 += relative; return *this; }

	inline Rect operator +(const Size &relative) { return Rect(p0 + relative, p1 + relative); }
	Rect &operator +=(const Size &relative) { p0 += relative; p1 += relative; return *this; }

	// Add or subtract border
	// Grow with the size of the border.
	inline Rect operator +(const Border &border) const { return Rect(p0.x - border.l, p0.y - border.t, p1.x + border.r, p1.y + border.b); }
	inline Rect &operator +=(const Border &border) { left -= border.l; top -= border.t; right += border.r; bottom += border.b; return *this; }
	// Shrink with the size of the border.
	inline Rect operator -(const Border &border) const { return Rect(p0.x + border.l, p0.y + border.t, p1.x - border.r, p1.y - border.b); }
	inline Rect &operator -=(const Border &border) { left += border.l; top += border.t; right -= border.r; bottom -= border.b; return *this; }

	// Get corners
	inline Point topLeft() const { return p0; }
	inline Point topRight() const { return Point(p1.x, p0.y); }
	inline Point bottomLeft() const { return Point(p0.x, p1.y); }
	inline Point bottomRight() const { return p1; }

	// Center points.
	inline Point topCenter() const { return Point((left + right) / 2, top); }
	inline Point leftCenter() const { return Point(left, (top + bottom) / 2); }
	inline Point rightCenter() const { return Point(right, (top + bottom) / 2); }
	inline Point bottomCenter() const { return Point((left + right) / 2, bottom); }

	// Does this rect contain the point "p"?
	bool contains(Point p) const;

	// Modification.
	void shrink(nat with);
	void grow(nat with);

	// Comparison
	inline bool operator ==(const Rect &o) const {
		if (p0 != o.p0) return false;
		if (p1 != o.p1) return false;
		return true;
	}
	inline bool operator !=(const Rect &o) const { return !(*this == o); }

	// Divide in pieces horizontally. The space is the default UI space of 7.
	Rect divideHoriz(int piece, int total, int space = 7);
};

bool intersects(const Rect &a, const Rect &b);
Rect intersection(const Rect &a, const Rect &b);

inline std::wostream &operator <<(std::wostream &to, const Rect &pt) {
	to << "(" << pt.p0 << " - " << pt.p1 << ")";
	return to;
}