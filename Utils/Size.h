#pragma once

#include "Border.h"

class Size {
public:
	int w, h;

	inline Size() : w(0), h(0) {};
	inline Size(int w, int h) : w(w), h(h) {}
	inline Size(CSize size) : w(size.cx), h(size.cy) {}

	// Expand any negative dimensions to the dimension given in "to"
	inline Size expanded(Size to) const {
		int w = this->w;
		int h = this->h;
		if (w < 0) w = to.w;
		if (h < 0) h = to.h;
		return Size(w, h);
	}

	inline Size operator +(const Border &border) const { return Size(w + border.width(), h + border.height()); }
	inline Size operator -(const Border &border) const { return Size(w - border.width(), h - border.height()); }

	inline Size operator +(const Size &size) const { return Size(w + size.w, h + size.h); }
	inline Size operator -(const Size &size) const { return Size(w - size.w, h - size.h); }

	inline Size operator *(int times) const { return Size(w * times, h * times); }
	inline Size operator /(int d) const { return Size(w / d, h / d); }

	inline Size &operator *=(int times) { w *= times; h *= times; return *this; }
	inline Size &operator /=(int d) { w /= d; h /= d; return *this; }

	inline bool operator ==(const Size &o) const { return (w == o.w) && (h == o.h); }
	inline bool operator !=(const Size &o) const { return (w != o.w) || (h != o.h); }

	inline float length() const { return sqrt(float(w * w + h * h)); }

	inline bool empty() const { return w < 0 && h < 0; }
};

// Max and min are always good to have!
inline Size max(const Size &a, const Size &b) {
	return Size(max(a.w, b.w), max(a.h, b.h));
}

inline Size min(const Size &a, const Size &b) {
	return Size(min(a.w, b.w), min(a.h, b.h));
}