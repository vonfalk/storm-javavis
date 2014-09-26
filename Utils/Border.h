#pragma once

// Represents a border in 2 dimensions.
class Border {
public:
	int l, t, r, b;

	inline Border() : t(0), l(0), r(0), b(0) {}
	inline Border(int l, int t, int r, int b) : l(l), t(t), r(r), b(b) {}
	inline Border(int border) : l(border), t(border), r(border), b(border) {}

	inline int width() const { return l + r; }
	inline int height() const { return t + b; }
};
