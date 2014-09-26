#pragma once

class Point;

// a simple point/vector in 3D-space. All code here is inlined for efficiency
// DO NOT CHANGE LAYOUT OR SIZE OF THIS OBJECT. IT IS ASSUMED TO BE EXACTLY AS DirectX WANTS IT.
class Vector {
public:
	float x, y, z;

	inline Vector(float x, float y, float z) : x(x), y(y), z(z) {};
	inline Vector() : x(0.0f), y(0.0f), z(0.0f) {}
	Vector(Point pt);

	// operators
	inline Vector operator -() const { return Vector(-x, -y, -z); }
	inline Vector operator +(const Vector &o) const { return Vector(x + o.x, y + o.y, z + o.z); }
	inline Vector operator -(const Vector &o) const { return Vector(x - o.x, y - o.y, z - o.z); }
	inline Vector operator *(float c) const { return Vector(x * c, y * c, z * c); }
	inline Vector operator /(float c) const { return Vector(x / c, y / c, z / c); }

	inline Vector &operator +=(const Vector &o) { x += o.x; y += o.y; z += o.z; return *this; }
	inline Vector &operator -=(const Vector &o) { x -= o.x; y -= o.y; z -= o.z; return *this; }
	inline Vector &operator *=(float v) { x *= v; y *= v; z *= v; return *this; }
	inline Vector &operator /=(float v) { v = 1.0f / v; x *= v; y *= v; z *= v; return *this; }

	// to dx format
	inline const float *toDx() const { return &x; }

	// length
	inline float lengthSq() const { return x*x + y*y + z*z; }
	inline float length() const { return sqrt(lengthSq()); }

	// normalization
	inline Vector normalize() const { return *this * (1 / length()); }

	inline static Vector zero() { return Vector(0, 0, 0); }

	// dot and cross product
	inline float operator *(const Vector &o) const { return x*o.x + y*o.y + z*o.z; }
	inline Vector operator /(const Vector &o) const { return Vector(y*o.z - z*o.y, z*o.x - x*o.z, x*o.y - y*o.x); }

	inline bool operator ==(const Vector &o) const { return (x == o.x) && (y == o.y) && (z == o.z); }
	inline bool operator !=(const Vector &o) const { return !(*this == o); }
};

std::wostream &operator <<(std::wostream &to, const Vector &pt);

inline bool eq(const Vector &a, const Vector &b, float tolerance = defTolerance) {
	return ::eq(a.x, b.x, tolerance) && ::eq(a.y, b.y, tolerance) && ::eq(a.z, b.z, tolerance);
}
