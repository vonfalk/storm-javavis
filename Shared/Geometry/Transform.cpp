#include "stdafx.h"
#include "Transform.h"

namespace storm {
	namespace geometry {

		Transform::Transform() {
			for (nat r = 0; r < 4; r++)
				for (nat c = 0; c < 4; c++)
					v[r][c] = (r == c) ? 1.0f : 0.0f;
		}

		Transform::Transform(Par<Transform> o) {
			memcpy(v, o->v, sizeof(v));
		}

		Transform::Transform(Float src[16]) {
			nat i = 0;
			for (nat r = 0; r < 4; r++)
				for (nat c = 0; c < 4; c++)
					v[r][c] = src[i++];
		}

		Transform::Transform(Float src[4][4]) {
			memcpy(v, src, sizeof(v));
		}

		Transform *Transform::operator *(Par<Transform> o) {
			float r[4][4];
			for (nat i = 0; i < 4; i++) {
				for (nat j = 0; j < 4; j++) {
					float sum = 0;
					for (int k = 0; k < 4; k++)
						sum += v[i][k] * o->v[k][j];
					r[i][j] = sum;
				}
			}
			return CREATE(Transform, this, r);
		}

		Vector Transform::operator *(Vector o) {
			float x = v[0][0]*o.x + v[0][1]*o.y + v[0][2]*o.z + v[0][3];
			float y = v[1][0]*o.x + v[1][1]*o.y + v[1][2]*o.z + v[1][3];
			float z = v[2][0]*o.x + v[2][1]*o.y + v[2][2]*o.z + v[2][3];
			float w = v[3][0]*o.x + v[3][1]*o.y + v[3][2]*o.z + v[3][3];
			return Vector(x/w, y/w, z/w);
		}

		Point Transform::operator *(Point o) {
			float x = v[0][0]*o.x + v[0][1]*o.y + v[0][3];
			float y = v[1][0]*o.x + v[1][1]*o.y + v[1][3];
			float w = v[3][0]*o.x + v[3][1]*o.y + v[3][3];
			return Point(x/w, y/w);
		}

		Transform *Transform::inverted() {
			Float inv[4][4];

			inv[0][0] = v[1][1] * v[2][2] * v[3][3] -
				v[1][1] * v[3][2] * v[2][3] -
				v[1][2] * v[2][1] * v[3][3] +
				v[1][2] * v[3][1] * v[2][3] +
				v[1][3] * v[2][1] * v[3][2] -
				v[1][3] * v[3][1] * v[2][2];

			inv[0][1] = -v[0][1] * v[2][2] * v[3][3] +
				v[0][1] * v[3][2] * v[2][3] +
				v[0][2] * v[2][1] * v[3][3] -
				v[0][2] * v[3][1] * v[2][3] -
				v[0][3] * v[2][1] * v[3][2] +
				v[0][3] * v[3][1] * v[2][2];

			inv[0][2] = v[0][1] * v[1][2] * v[3][3] -
				v[0][1] * v[3][2] * v[1][3] -
				v[0][2] * v[1][1] * v[3][3] +
				v[0][2] * v[3][1] * v[1][3] +
				v[0][3] * v[1][1] * v[3][2] -
				v[0][3] * v[3][1] * v[1][2];

			inv[0][3] = -v[0][1] * v[1][2] * v[2][3] +
				v[0][1] * v[2][2] * v[1][3] +
				v[0][2] * v[1][1] * v[2][3] -
				v[0][2] * v[2][1] * v[1][3] -
				v[0][3] * v[1][1] * v[2][2] +
				v[0][3] * v[2][1] * v[1][2];

			inv[1][0] = -v[1][0] * v[2][2] * v[3][3] +
				v[1][0] * v[3][2] * v[2][3] +
				v[1][2] * v[2][0] * v[3][3] -
				v[1][2] * v[3][0] * v[2][3] -
				v[1][3] * v[2][0] * v[3][2] +
				v[1][3] * v[3][0] * v[2][2];

			inv[1][1] = v[0][0] * v[2][2] * v[3][3] -
				v[0][0] * v[3][2] * v[2][3] -
				v[0][2] * v[2][0] * v[3][3] +
				v[0][2] * v[3][0] * v[2][3] +
				v[0][3] * v[2][0] * v[3][2] -
				v[0][3] * v[3][0] * v[2][2];

			inv[1][2] = -v[0][0] * v[1][2] * v[3][3] +
				v[0][0] * v[3][2] * v[1][3] +
				v[0][2] * v[1][0] * v[3][3] -
				v[0][2] * v[3][0] * v[1][3] -
				v[0][3] * v[1][0] * v[3][2] +
				v[0][3] * v[3][0] * v[1][2];

			inv[1][3] = v[0][0] * v[1][2] * v[2][3] -
				v[0][0] * v[2][2] * v[1][3] -
				v[0][2] * v[1][0] * v[2][3] +
				v[0][2] * v[2][0] * v[1][3] +
				v[0][3] * v[1][0] * v[2][2] -
				v[0][3] * v[2][0] * v[1][2];

			inv[2][0] = v[1][0] * v[2][1] * v[3][3] -
				v[1][0] * v[3][1] * v[2][3] -
				v[1][1] * v[2][0] * v[3][3] +
				v[1][1] * v[3][0] * v[2][3] +
				v[1][3] * v[2][0] * v[3][1] -
				v[1][3] * v[3][0] * v[2][1];

			inv[2][1] = -v[0][0] * v[2][1] * v[3][3] +
				v[0][0] * v[3][1] * v[2][3] +
				v[0][1] * v[2][0] * v[3][3] -
				v[0][1] * v[3][0] * v[2][3] -
				v[0][3] * v[2][0] * v[3][1] +
				v[0][3] * v[3][0] * v[2][1];

			inv[2][2] = v[0][0] * v[1][1] * v[3][3] -
				v[0][0] * v[3][1] * v[1][3] -
				v[0][1] * v[1][0] * v[3][3] +
				v[0][1] * v[3][0] * v[1][3] +
				v[0][3] * v[1][0] * v[3][1] -
				v[0][3] * v[3][0] * v[1][1];

			inv[2][3] = -v[0][0] * v[1][1] * v[2][3] +
				v[0][0] * v[2][1] * v[1][3] +
				v[0][1] * v[1][0] * v[2][3] -
				v[0][1] * v[2][0] * v[1][3] -
				v[0][3] * v[1][0] * v[2][1] +
				v[0][3] * v[2][0] * v[1][1];

			inv[3][0] = -v[1][0] * v[2][1] * v[3][2] +
				v[1][0] * v[3][1] * v[2][2] +
				v[1][1] * v[2][0] * v[3][2] -
				v[1][1] * v[3][0] * v[2][2] -
				v[1][2] * v[2][0] * v[3][1] +
				v[1][2] * v[3][0] * v[2][1];

			inv[3][1] = v[0][0] * v[2][1] * v[3][2] -
				v[0][0] * v[3][1] * v[2][2] -
				v[0][1] * v[2][0] * v[3][2] +
				v[0][1] * v[3][0] * v[2][2] +
				v[0][2] * v[2][0] * v[3][1] -
				v[0][2] * v[3][0] * v[2][1];

			inv[3][2] = -v[0][0] * v[1][1] * v[3][2] +
				v[0][0] * v[3][1] * v[1][2] +
				v[0][1] * v[1][0] * v[3][2] -
				v[0][1] * v[3][0] * v[1][2] -
				v[0][2] * v[1][0] * v[3][1] +
				v[0][2] * v[3][0] * v[1][1];

			inv[3][3] = v[0][0] * v[1][1] * v[2][2] -
				v[0][0] * v[2][1] * v[1][2] -
				v[0][1] * v[1][0] * v[2][2] +
				v[0][1] * v[2][0] * v[1][2] +
				v[0][2] * v[1][0] * v[2][1] -
				v[0][2] * v[2][0] * v[1][1];

			float det = v[0][0] * inv[0][0] + v[1][0] * inv[0][1] + v[2][0] * inv[0][2] + v[3][0] * inv[0][3];

			if (det == 0) {
				assert(false);
				return CREATE(Transform, this);
			}

			det = 1.0f / det;

			for (nat i = 0; i < 4; i++)
				for (nat j = 0; j < 4; j++)
					inv[i][j] *= det;

			return CREATE(Transform, this, inv);
		}

		Transform *translate(EnginePtr e, Vector v) {
			float d[] = {
				1, 0, 0, v.x,
				0, 1, 0, v.y,
				0, 0, 1, v.z,
				0, 0, 0, 1,
			};
			return CREATE(Transform, e.v, d);
		}

		Transform *rotateX(EnginePtr e, Float angle) {
			float s = sin(angle), c = cos(angle);
			float d[] = {
				1, 0, 0, 0,
				0, c, -s, 0,
				0, s, c, 0,
				0, 0, 0, 1,
			};
			return CREATE(Transform, e.v, d);
		}

		Transform *rotateY(EnginePtr e, Float angle) {
			float s = sin(angle), c = cos(angle);
			float d[] = {
				c, 0, s, 0,
				0, 1, 0, 0,
				-s, 0, c, 0,
				0, 0, 0, 1,
			};
			return CREATE(Transform, e.v, d);
		}

		Transform *rotateZ(EnginePtr e, Float angle) {
			float s = sin(angle), c = cos(angle);
			float d[] = {
				c, -s, 0, 0,
				s, c, 0, 0,
				0, 0, 1, 0,
				0, 0, 0, 1,
			};
			return CREATE(Transform, e.v, d);
		}

		Transform *rotate(EnginePtr e, Float angle) {
			return rotateZ(e, angle);
		}

		Transform *scale(EnginePtr e, Float scale) {
			float d[] = {
				scale, 0, 0, 0,
				0, scale, 0, 0,
				0, 0, scale, 0,
				0, 0, 0, 1,
			};
			return CREATE(Transform, e.v, d);
		}

		Transform *scale(EnginePtr e, Vector scale) {
			float d[] = {
				scale.x, 0, 0, 0,
				0, scale.y, 0, 0,
				0, 0, scale.z, 0,
				0, 0, 0, 1,
			};
			return CREATE(Transform, e.v, d);
		}

	}
}
