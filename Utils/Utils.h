#pragma once

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(*x))

#define _USE_MATH_DEFINES
#include <cmath>
#include <assert.h>

#undef min
#undef max

#include <algorithm>

const float pi = float(M_PI);

using std::min;
using std::max;
using std::swap;
using std::make_pair;

typedef unsigned int nat;
typedef unsigned short nat16;
typedef unsigned __int64 uint64;
typedef unsigned __int64 nat64;
typedef __int64 int64;
typedef unsigned char byte;

#include "Containers.h"

inline int64 abs(int64 v) {
	return v < 0 ? -v : v;
}

template <class T>
inline void limitMax(T &toLimit, const T &b) {
	toLimit = min(toLimit, b);
}

template <class T>
inline void limitMin(T &toLimit, const T &b) {
	toLimit = max(toLimit, b);
}

template <class T>
inline void limit(T &toLimit, const T &minLimit, const T &maxLimit) {
	toLimit = min(maxLimit, max(minLimit, toLimit));
}

#define null NULL

inline float degToRad(float angle) {
	return float(angle * M_PI / 180.0);
}

inline float radToDeg(float angle) {
	return float(angle * 180.0 / M_PI);
}


bool getAsyncKeyState(nat key); //Hur är det just nu?
bool getKeyState(nat key); //Hur var det när nuv. meddelande skickades?

bool positive(float f);

template <class T>
inline void *address(T fn) {
	return (void *&)fn;
}

//Delete an object, and clear the pointer to it.
template <class T>
inline void del(T *&ptr) {
	delete ptr;
	ptr = null;
}

// Clear the contents of a vector or list containing pointers.
template <class T>
inline void clear(T &vec) {
	while (vec.size() > 0) {
		delete vec.back();
		vec.pop_back();
	}
}

// Release a pointer to an interface.
template <class T>
inline void release(T *&ptr) {
	if (ptr) ptr->Release();
	ptr = null;
}


//Disable warning about using this pointer in ctor of objects since
//this occurs in almost every class using the member function pointer.
#pragma warning ( disable : 4355 )


//Clear an object. Equivalent to ZeroMemory(&t, sizeof(T));
template <class T>
inline void zeroMem(T &obj) {
	ZeroMemory(&obj, sizeof(T));
}

// clear an array
template <class T>
inline void zeroArray(T *arr, nat count) {
	ZeroMemory(arr, count * sizeof(T));
}

const float defTolerance = 0.01f;
inline float eq(float a, float b, float tolerance = defTolerance) {
	return a + tolerance >= b && a - tolerance <= b;
}

// Atomic increase and decrease of variables
void atomicIncrement(volatile nat &v);
void atomicDecrement(volatile nat &v);

// angle functions


// degrees to radians
inline float deg(float inDeg) { return inDeg * pi / 180.0f; }

// radians to degrees
inline float toDeg(float inRad) { return inRad * 180.0f / pi; }

#include "Object.h"
#include "Debug.h"
