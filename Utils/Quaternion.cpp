#include "StdAfx.h"
#include "Quaternion.h"

#include "Point.h"

Quaternion::Quaternion(const Vector &axis, float angle) {
	Vector a = axis.normalize();

	float s = sin(0.5f * angle);
	x = s * a.x;
	y = s * a.y;
	z = s * a.z;
	w = cos(0.5f * angle);
}

float Quaternion::length() const {
	return sqrt(lengthSq());
}

float Quaternion::lengthSq() const {
	return x*x + y*y + z*z + w*w;
}

Quaternion Quaternion::normalize() const {
	return *this * (1.0f / lengthSq());
}

Quaternion Quaternion::operator *(float v) const {
	return Quaternion(x * v, y * v, z * v, w * v);
}
