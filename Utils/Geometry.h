#pragma once

#include "Vector.h"
#include "Point.h"

namespace util {

	// File with geometry helpers.

	// Bezier point calculation.
	Vector bezierPoint(const vector<Vector> &pts, float param);

	// Bezier collision results.
	struct BezierDistance {
		// The parameter (0-1) at the collision.
		float parameter;

		// The distance to the tested point.
		float distance;
	};

	// Check collision with a bezier curve (4 points). The step specifies how fine the search shall be.
	BezierDistance bezierDistance(const vector<Vector> &pts, const Vector &test, float step = 0.01f);
	BezierDistance bezierDistance(const vector<Point> &pts, const Point &test, float step = 0.01f);

}