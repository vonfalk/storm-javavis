#pragma once

class Vector;

// quaternion rotation, must have the layout specified here
class Quaternion {
public:
	float x, y, z, w;

	inline Quaternion() : x(0.0f), y(0.0f), z(0.0f), w(0.0f) {}

	Quaternion(const Vector &axis, float angle);

	inline Quaternion(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}

	float length() const;
	float lengthSq() const;
	Quaternion normalize() const;

	Quaternion operator *(float v) const;
};
