#include "StdAfx.h"
#include "Color.h"

#include "Point.h"
#include "Stream.h"

Color::Color() : r(0), g(0), b(0), a(1.0f) {}

Color::Color(float r, float g, float b, float a) : r(r), g(g), b(b), a(a) {}

Color::Color(COLORREF color, float a) : a(a) {
	b = float((color >> 16) & 0xFF) / 255.0f;
	g = float((color >> 8) & 0xFF) / 255.0f;
	r = float(color & 0xFF) / 255.0f;
}

Color::Color(const byte *src) {
	r = src[0] / 255.0f;
	g = src[1] / 255.0f;
	b = src[2] / 255.0f;
	a = src[3] / 255.0f;
}

void Color::save(byte *to) const {
	to[0] = byte(255.0f * r);
	to[1] = byte(255.0f * g);
	to[2] = byte(255.0f * b);
	to[3] = byte(255.0f * a);
}

Color &Color::operator =(const Vector &pt) {
	r = pt.x;
	g = pt.y;
	b = pt.z;
	a = 1.0f;
	return *this;
}

std::wostream &operator <<(std::wostream &to, const Color &c) {
	to << L"{" << c.r << L", " << c.g << L", " << c.b << L", " << c.a << L"}";
	return to;
}

COLORREF Color::ref() const {
	COLORREF z = byte(255.0f * r);
	z |= byte(255.0f * g) << 8;
	z |= byte(255.0f * b) << 16;
	return z;
}

Color Color::sysColor(int id) {
	COLORREF c = GetSysColor(id);
	return Color(c, 1.0f);
}

Color Color::load(util::Stream &from) {
	float r = from.read<float>();
	float g = from.read<float>();
	float b = from.read<float>();
	float a = from.read<float>();
	return Color(r, g, b, a);
}

void Color::save(util::Stream &to) const {
	to.write<float>(r);
	to.write<float>(g);
	to.write<float>(b);
	to.write<float>(a);
}