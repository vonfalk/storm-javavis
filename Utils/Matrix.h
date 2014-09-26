#pragma once

#include "Quaternion.h"

#include "Point.h"
#include "Size.h"

// Simple matrix class for managing affine transforms in 3 dimensions.
class Matrix {
public:
	// row, column. Just like D3DMATRIX
	float v[4][4];

	// get a pointer to the first element, as needed by directX
	inline const float *toDX() const { return &v[0][0]; }

	// convenient access
	inline float *operator [](int row) { return v[row]; }
	inline const float *operator[](int row) const { return v[row]; }

	// scale matrix
	Matrix operator *(float v) const;

	// add matrices
	Matrix operator +(const Matrix &o) const;

	// matrix multiplication
	Matrix operator *(const Matrix &o) const;

	// transform the point (pt, 1.0) and return (x/w, y/w, z/w)
	Vector operator *(const Vector &pt) const;

	// transform a normal by transforming two points
	Vector transformNormal(const Vector &pt) const;

	// other useful operations
	Matrix invert() const;
	Matrix transpose() const;
	float det() const;

	// create various matrices
	static Matrix zero();
	static Matrix unit();
	static Matrix scale(float factor);
	static Matrix scale(const Vector &factors);
	static Matrix translate(const Vector &pt);
	static inline Matrix translate(float x, float y, float z) { return translate(Vector(x, y, z)); }

	// create rotation matrices
	static Matrix rotateX(float angle);
	static Matrix rotateY(float angle);
	static Matrix rotateZ(float angle);
	static Matrix rotate(const Quaternion &r);

	// projection matrix creation
	static Matrix perspectiveFovRH(float fovy, float aspect, float zNear, float zFar);

	// Matrix for screen space creation. This transforms the screen coordinates to (0..1) in x and y direction, assumes w = 1
	static Matrix dxScreenView(Size size);
};

inline Matrix operator *(float scalar, const Matrix &m) { return m * scalar; }

// output a matrix, will output a line ending after the output
std::wostream &operator <<(std::wostream &to, const Matrix &m);

bool eq(const Matrix &a, const Matrix &b, float tolerance = defTolerance);
