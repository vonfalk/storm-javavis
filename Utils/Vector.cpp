#include "stdafx.h"
#include "Vector.h"

#include "Point.h"

#include <iostream>

Vector::Vector(Point pt) : x(float(pt.x)), y(float(pt.y)), z(0.0f) {}

std::wostream &operator <<(std::wostream &to, const Vector &pt) {
	to << L"(" << pt.x << L", " << pt.y << L", " << pt.z << L")";
	return to;
}
