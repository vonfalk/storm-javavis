#include "StdAfx.h"
#include "Geometry.h"

namespace util {

	Vector bezierPoint(const vector<Vector> &pts, float param) {
		assert(pts.size() == 4);

		Vector r = pts[0] * pow(1 - param, 3);
		r += pts[1] * 3 * pow(1 - param, 2) * param;
		r += pts[2] * 3 * (1 - param) * param * param;
		r += pts[3] * pow(param, 3);
		return r;
	}

	BezierDistance bezierDistance(const vector<Vector> &pts, const Vector &test, float stepSz) {
		BezierDistance dist;
		dist.distance = (bezierPoint(pts, 0) - test).lengthSq();
		dist.parameter = 0;

		int steps = int(1 / stepSz);
		for (int i = 1; i <= steps; i++) {
			float at = i * stepSz;
			float distance = (bezierPoint(pts, at) - test).lengthSq();

			if (distance < dist.distance) {
				dist.distance = distance;
				dist.parameter = at;
			}
		}

		dist.distance = sqrt(dist.distance);
		return dist;
	}

	BezierDistance bezierDistance(const vector<Point> &pts, const Point &test, float stepSz) {
		vector<Vector> v(pts.begin(), pts.end());
		return bezierDistance(v, Vector(test), stepSz);
	}
}
