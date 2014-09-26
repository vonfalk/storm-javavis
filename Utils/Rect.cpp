#include "stdafx.h"
#include "Rect.h"


void Rect::normalize() {
	if (p0.x > p1.x) std::swap(p0.x, p1.x);
	if (p0.y > p1.y) std::swap(p0.y, p1.y);
}

bool Rect::contains(Point p) const {
	if (p.x < p0.x) return false;
	if (p.x >= p1.x) return false;
	if (p.y < p0.y) return false;
	if (p.y >= p1.y) return false;
	return true;
}

void Rect::shrink(nat with) {
	p0.x += with;
	p0.y += with;
	p1.x -= with;
	p1.y -= with;
}

void Rect::grow(nat with) {
	p0.x -= with;
	p0.y -= with;
	p1.x += with;
	p1.y += with;
}

Rect Rect::divideHoriz(int piece, int total, int space) {
	Rect rect = *this;
	int pieceWidth = (rect.width() - (total - 1) * space) / total;
	rect.p0.x += (pieceWidth + space) * piece;
	rect.p1.x = rect.p0.x + pieceWidth;
	return rect;
}

bool intersects(const Rect &a, const Rect &b) {
	RECT r;
	return IntersectRect(&r, &a.win(), &b.win()) == TRUE;
}

Rect intersection(const Rect &a, const Rect &b) {
	RECT r;
	IntersectRect(&r, &a.win(), &b.win());
	return Rect(r);
}