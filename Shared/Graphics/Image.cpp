#include "stdafx.h"
#include "Image.h"

namespace storm {
	using namespace geometry;

	inline Color toColor(byte *src) {
		return Color(src[0], src[1], src[2], src[3]);
	}

	inline byte toByte(float f) {
		return byte(f * 255.0f);
	}

	inline void fromColor(const Color &src, byte *dest) {
		dest[0] = toByte(src.r);
		dest[1] = toByte(src.g);
		dest[2] = toByte(src.b);
		dest[3] = toByte(src.a);
	}

	Image::Image() : data(null), w(0), h(0) {}

	Image::Image(Size size) : data(null), w(nat(size.w)), h(nat(size.h)) {
		nat s = w * h * 4;
		data = new byte[s];
		memset(data, 0, s);
	}

	Image::Image(Nat w, Nat h) : data(null), w(w), h(h) {
		nat s = w * h * 4;
		data = new byte[s];
		memset(data, 0, s);
	}

	Image::~Image() {
		delete []data;
	}

	geometry::Size Image::size() {
		return geometry::Size(Float(w), Float(h));
	}

	nat Image::offset(nat x, nat y) {
		return (y * w + x) * 4;
	}

	Color Image::get(Nat x, Nat y) {
		if (x >= w || y >= h)
			return Color();
		return toColor(data + offset(x, y));
	}

	Color Image::get(Point p) {
		// TODO? Interpolate colors?
		return get(nat(p.x), nat(p.y));
	}

	void Image::set(Nat x, Nat y, Color c) {
		if (x >= w || y >= h)
			return;
		fromColor(c, data + offset(x, y));
	}

	void Image::set(Point p, Color c) {
		set(nat(p.x), nat(p.y), c);
	}

	nat Image::stride() const {
		return w * 4;
	}

	nat Image::bufferSize() const {
		return w * h * 4;
	}

	byte *Image::buffer() {
		return data;
	}

	byte *Image::buffer(nat x, nat y) {
		return data + offset(x, y);
	}


}
