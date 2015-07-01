#include "stdafx.h"
#include "Vector.h"
#include "Str.h"

namespace storm {
	namespace geometry {

		Vector::Vector() : x(0), y(0), z(0) {}

		Vector::Vector(Float x, Float y, Float z) : x(x), y(y), z(z) {}

		Vector::Vector(Point p) : x(p.x), y(p.y), z(0) {}

		wostream &operator <<(wostream &to, const Vector &s) {
			return to << L"(" << s.x << L", " << s.y << L", " << s.z << L")";
		}

		Str *toS(EnginePtr e, Vector s) {
			return CREATE(Str, e.v, ::toS(s));
		}

		Vector operator +(Vector a, Vector b) {
			return Vector(a.x + b.x, a.y + b.y, a.z + b.z);
		}

		Vector operator -(Vector a, Vector b) {
			return Vector(a.x - b.x, a.y - b.y, a.z - b.z);
		}

		Vector operator *(Vector a, Float b) {
			return Vector(a.x * b, a.y * b, a.z * b);
		}

		Vector operator *(Float a, Vector b) {
			return Vector(a * b.x, a * b.y, a * b.z);
		}

		Vector operator /(Vector a, Float b) {
			return Vector(a.x / b, a.y / b, a.z / b);
		}

		// Dot and cross product.
		Vector operator *(Vector a, Vector b) {
			return Vector(a.x * b.x, a.y * b.y, a.z * b.z);
		}

		Vector operator /(Vector a, Vector b) {
			return Vector(a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.z*b.y - a.y*b.z);
		}

		Vector abs(Vector a) {
			return Vector(::abs(a.x), ::abs(a.y), ::abs(a.z));
		}

	}
}
