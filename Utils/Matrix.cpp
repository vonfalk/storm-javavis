#include "stdafx.h"
#include "Matrix.h"

#include <iomanip>


bool eq(const Matrix &a, const Matrix &b, float tolerance /* = defTolerance */) {
	for (int r = 0; r < 4; r++) {
		for (int c = 0; c < 4; c++) {
			if (!::eq(a.v[r][c], b.v[r][c], tolerance)) return false;
		}
	}
	return true;
}

Matrix Matrix::operator *(float s) const {
	Matrix result;
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			result[i][j] = v[i][j] * s;
		}
	}
	return result;
}


Matrix Matrix::operator +(const Matrix &o) const {
	Matrix result;

	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			result[i][j] = v[i][j] + o[i][j];
		}
	}

	return result;
}


Matrix Matrix::operator *(const Matrix &o) const {
	Matrix result;

	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			float sum = 0;
			for (int k = 0; k < 4; k++) {
				sum += v[i][k] * o[k][j];
			}
			result[i][j] = sum;
		}
	}

	return result;
}


Vector Matrix::operator *(const Vector &pt) const {
	float x = pt.x * v[0][0] + pt.y * v[0][1] + pt.z * v[0][2] + v[0][3];
	float y = pt.x * v[1][0] + pt.y * v[1][1] + pt.z * v[1][2] + v[1][3];
	float z = pt.x * v[2][0] + pt.y * v[2][1] + pt.z * v[2][2] + v[2][3];
	float w = pt.x * v[3][0] + pt.y * v[3][1] + pt.z * v[3][2] + v[3][3];
	
	return Vector(x/w, y/w, z/w);
}


Matrix Matrix::transpose() const {
	Matrix result;
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			result[j][i] = v[i][j];
		}
	}
	return result;
}

Matrix Matrix::invert() const {
	Matrix inv;
	float det;

	inv.v[0][0] = v[1][1] * v[2][2] * v[3][3] - 
		v[1][1] * v[3][2] * v[2][3] - 
		v[1][2] * v[2][1] * v[3][3] + 
		v[1][2] * v[3][1] * v[2][3] +
		v[1][3] * v[2][1] * v[3][2] - 
		v[1][3] * v[3][1] * v[2][2];

	inv.v[0][1] = -v[0][1] * v[2][2] * v[3][3] + 
		v[0][1] * v[3][2] * v[2][3] + 
		v[0][2] * v[2][1] * v[3][3] - 
		v[0][2] * v[3][1] * v[2][3] - 
		v[0][3] * v[2][1] * v[3][2] + 
		v[0][3] * v[3][1] * v[2][2];

	inv.v[0][2] = v[0][1] * v[1][2] * v[3][3] - 
		v[0][1] * v[3][2] * v[1][3] - 
		v[0][2] * v[1][1] * v[3][3] + 
		v[0][2] * v[3][1] * v[1][3] + 
		v[0][3] * v[1][1] * v[3][2] - 
		v[0][3] * v[3][1] * v[1][2];

	inv.v[0][3] = -v[0][1] * v[1][2] * v[2][3] + 
		v[0][1] * v[2][2] * v[1][3] +
		v[0][2] * v[1][1] * v[2][3] - 
		v[0][2] * v[2][1] * v[1][3] - 
		v[0][3] * v[1][1] * v[2][2] + 
		v[0][3] * v[2][1] * v[1][2];

	inv.v[1][0] = -v[1][0] * v[2][2] * v[3][3] + 
		v[1][0] * v[3][2] * v[2][3] + 
		v[1][2] * v[2][0] * v[3][3] - 
		v[1][2] * v[3][0] * v[2][3] - 
		v[1][3] * v[2][0] * v[3][2] + 
		v[1][3] * v[3][0] * v[2][2];

	inv.v[1][1] = v[0][0] * v[2][2] * v[3][3] - 
		v[0][0] * v[3][2] * v[2][3] - 
		v[0][2] * v[2][0] * v[3][3] + 
		v[0][2] * v[3][0] * v[2][3] + 
		v[0][3] * v[2][0] * v[3][2] - 
		v[0][3] * v[3][0] * v[2][2];

	inv.v[1][2] = -v[0][0] * v[1][2] * v[3][3] + 
		v[0][0] * v[3][2] * v[1][3] + 
		v[0][2] * v[1][0] * v[3][3] - 
		v[0][2] * v[3][0] * v[1][3] - 
		v[0][3] * v[1][0] * v[3][2] + 
		v[0][3] * v[3][0] * v[1][2];

	inv.v[1][3] = v[0][0] * v[1][2] * v[2][3] - 
		v[0][0] * v[2][2] * v[1][3] - 
		v[0][2] * v[1][0] * v[2][3] + 
		v[0][2] * v[2][0] * v[1][3] + 
		v[0][3] * v[1][0] * v[2][2] - 
		v[0][3] * v[2][0] * v[1][2];

	inv.v[2][0] = v[1][0] * v[2][1] * v[3][3] - 
		v[1][0] * v[3][1] * v[2][3] - 
		v[1][1] * v[2][0] * v[3][3] + 
		v[1][1] * v[3][0] * v[2][3] + 
		v[1][3] * v[2][0] * v[3][1] - 
		v[1][3] * v[3][0] * v[2][1];

	inv.v[2][1] = -v[0][0] * v[2][1] * v[3][3] + 
		v[0][0] * v[3][1] * v[2][3] + 
		v[0][1] * v[2][0] * v[3][3] - 
		v[0][1] * v[3][0] * v[2][3] - 
		v[0][3] * v[2][0] * v[3][1] + 
		v[0][3] * v[3][0] * v[2][1];

	inv.v[2][2] = v[0][0] * v[1][1] * v[3][3] - 
		v[0][0] * v[3][1] * v[1][3] - 
		v[0][1] * v[1][0] * v[3][3] + 
		v[0][1] * v[3][0] * v[1][3] + 
		v[0][3] * v[1][0] * v[3][1] - 
		v[0][3] * v[3][0] * v[1][1];

	inv.v[2][3] = -v[0][0] * v[1][1] * v[2][3] + 
		v[0][0] * v[2][1] * v[1][3] + 
		v[0][1] * v[1][0] * v[2][3] - 
		v[0][1] * v[2][0] * v[1][3] - 
		v[0][3] * v[1][0] * v[2][1] + 
		v[0][3] * v[2][0] * v[1][1];

	inv.v[3][0] = -v[1][0] * v[2][1] * v[3][2] + 
		v[1][0] * v[3][1] * v[2][2] + 
		v[1][1] * v[2][0] * v[3][2] - 
		v[1][1] * v[3][0] * v[2][2] - 
		v[1][2] * v[2][0] * v[3][1] + 
		v[1][2] * v[3][0] * v[2][1];

	inv.v[3][1] = v[0][0] * v[2][1] * v[3][2] - 
		v[0][0] * v[3][1] * v[2][2] - 
		v[0][1] * v[2][0] * v[3][2] + 
		v[0][1] * v[3][0] * v[2][2] + 
		v[0][2] * v[2][0] * v[3][1] - 
		v[0][2] * v[3][0] * v[2][1];

	inv.v[3][2] = -v[0][0] * v[1][1] * v[3][2] + 
		v[0][0] * v[3][1] * v[1][2] + 
		v[0][1] * v[1][0] * v[3][2] - 
		v[0][1] * v[3][0] * v[1][2] - 
		v[0][2] * v[1][0] * v[3][1] + 
		v[0][2] * v[3][0] * v[1][1];

	inv.v[3][3] = v[0][0] * v[1][1] * v[2][2] - 
		v[0][0] * v[2][1] * v[1][2] - 
		v[0][1] * v[1][0] * v[2][2] + 
		v[0][1] * v[2][0] * v[1][2] + 
		v[0][2] * v[1][0] * v[2][1] - 
		v[0][2] * v[2][0] * v[1][1];

	det = v[0][0] * inv.v[0][0] + v[1][0] * inv.v[0][1] + v[2][0] * inv.v[0][2] + v[3][0] * inv.v[0][3];

	if (det == 0) {
		ASSERT(FALSE);
		return zero();
	}

	det = 1.0f / det;

	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			inv.v[i][j] *= det;
		}
	}

	return inv;
}

float Matrix::det() const {
	float inv00, inv01, inv02, inv03;

	inv00 = v[1][1] * v[2][2] * v[3][3] - 
		v[1][1] * v[3][2] * v[2][3] - 
		v[1][2] * v[2][1] * v[3][3] + 
		v[1][2] * v[3][1] * v[2][3] +
		v[1][3] * v[2][1] * v[3][2] - 
		v[1][3] * v[3][1] * v[2][2];

	inv01 = -v[0][1] * v[2][2] * v[3][3] + 
		v[0][1] * v[3][2] * v[2][3] + 
		v[0][2] * v[2][1] * v[3][3] - 
		v[0][2] * v[3][1] * v[2][3] - 
		v[0][3] * v[2][1] * v[3][2] + 
		v[0][3] * v[3][1] * v[2][2];

	inv02 = v[0][1] * v[1][2] * v[3][3] - 
		v[0][1] * v[3][2] * v[1][3] - 
		v[0][2] * v[1][1] * v[3][3] + 
		v[0][2] * v[3][1] * v[1][3] + 
		v[0][3] * v[1][1] * v[3][2] - 
		v[0][3] * v[3][1] * v[1][2];

	inv03 = -v[0][1] * v[1][2] * v[2][3] + 
		v[0][1] * v[2][2] * v[1][3] +
		v[0][2] * v[1][1] * v[2][3] - 
		v[0][2] * v[2][1] * v[1][3] - 
		v[0][3] * v[1][1] * v[2][2] + 
		v[0][3] * v[2][1] * v[1][2];

	return v[0][0] * inv00 + v[1][0] * inv01 + v[2][0] * inv02 + v[3][0] * inv03;
}

Vector Matrix::transformNormal(const Vector &pt) const {
	Vector z = *this * Vector(0, 0, 0);
	Vector at = *this * pt;
	return (at - z).normalize();
}

Matrix Matrix::zero() {
	Matrix m;
	zeroMem(m);
	return m;
}

Matrix Matrix::unit() {
	Matrix m = zero();
	for (int i = 0; i < 4; i++) m[i][i] = 1.0f;
	return m;
}

Matrix Matrix::scale(float scale) {
	Matrix m;
	zeroMem(m);
	for (int i = 0; i < 3; i++) m[i][i] = scale;
	m[3][3] = 1.0f;
	return m;
}

Matrix Matrix::scale(const Vector &factors) {
	Matrix m;
	zeroMem(m);
	m[0][0] = factors.x;
	m[1][1] = factors.y;
	m[2][2] = factors.z;
	m[3][3] = 1.0f;
	return m;
}

Matrix Matrix::translate(const Vector &pt) {
	Matrix m(unit());
	m.v[0][3] = pt.x;
	m.v[1][3] = pt.y;
	m.v[2][3] = pt.z;
	return m;
}

Matrix Matrix::rotateX(float angle) {
	Matrix m = unit();
	m.v[2][2] = m.v[1][1] = cos(angle);
	m.v[2][1] = sin(angle);
	m.v[1][2] = -m.v[2][1];
	return m;
}

Matrix Matrix::rotateY(float angle) {
	Matrix m = unit();
	m.v[0][0] = m.v[2][2] = cos(angle);
	m.v[0][2] = sin(angle);
	m.v[2][0] = -m.v[0][2];
	return m;
}

Matrix Matrix::rotateZ(float angle) {
	Matrix m = unit();
	m.v[0][0] = m.v[1][1] = cos(angle);
	m.v[1][0] = sin(angle);
	m.v[0][1] = -m.v[1][0];
	return m;
}

Matrix Matrix::rotate(const Quaternion &r) {
	Matrix m;
	zeroMem(m);

	float x = r.x, y = r.y, z = r.z, w = r.w,
		tx = 2*x, ty = 2*y, tz = 2*z,
		txx = tx*x, tyy = ty*y, tzz = tz*z,
		txy = tx*y, txz = tx*z, tyz = ty*z,
		twx = w*tx, twy = w*ty, twz = w*tz;

	m[0][0] = 1 - (tyy + tzz);
	m[0][1] = txy - twz;
	m[0][2] = txz + twy;
	
	m[1][0] = txy + twz;
	m[1][1] = 1 - (txx + tzz);
	m[1][2] = tyz - twx;

	m[2][0] = txz - twy;
	m[2][1] = tyz + twx;
	m[2][2] = 1 - (txx + tyy);

	m[3][3] = 1.0f;

	return m;
}

Matrix Matrix::perspectiveFovRH(float fovy, float aspect, float zNear, float zFar) {
	Matrix m;
	zeroMem(m);
	float yScale = 1 / tan(fovy / 2);
	float xScale = yScale / aspect;
	float zScale = zFar / (zNear - zFar);

	m[0][0] = xScale;
	m[1][1] = yScale;
	m[2][2] = zScale;
	m[2][3] = zNear * zScale;
	m[3][2] = -1.0f;

	return m;
}

Matrix Matrix::dxScreenView(Size size) {
	Matrix m(unit());

	m[0][0] = 2.0f / size.w;
	m[0][3] = -1.0f;
	m[1][1] = -2.0f / size.h;
	m[1][3] = 1.0f;

	return m;
}

// printing
std::wostream &operator <<(std::wostream &to, const Matrix &m) {
	for (int r = 0; r < 4; r++) {
		for (int c = 0; c < 4; c++) {
			float v = m.v[r][c];
			if (v >= 0.0f) {
				to << " ";
			}
			to << std::setw(4) << std::setprecision(2) << std::fixed << m.v[r][c] << " ";
		}
		to << std::endl;
	}
	return to;
}