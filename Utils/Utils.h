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
typedef wchar_t wchar;

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

// Hack to allow casting member-function-pointers into void *.
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
	for (T::iterator i = vec.begin(), end = vec.end(); i != end; ++i)
		delete *i;
	vec.clear();
}

// Clear the contents of a map<?, ?*>.
template <class T>
inline void clearMap(T &map) {
	for (T::iterator i = map.begin(), end = map.end(); i != end; ++i)
		delete i->second;
	map.clear();
}

//Disable warning about using this pointer in ctor of objects since
//this occurs in almost every class using the member function pointer.
#pragma warning ( disable : 4355 )


//Clear an object. Equivalent to ZeroMemory(&t, sizeof(T));
template <class T>
inline void zeroMem(T &obj) {
	memset(&obj, 0, sizeof(T));
}

// clear an array
template <class T>
inline void zeroArray(T *arr, nat count) {
	memset(arr, 0, count * sizeof(T));
}

const float defTolerance = 0.01f;
inline float eq(float a, float b, float tolerance = defTolerance) {
	return a + tolerance >= b && a - tolerance <= b;
}

// Atomic increase and decrease of variables
nat atomicIncrement(volatile nat &v);
nat atomicDecrement(volatile nat &v);

// angle functions


// degrees to radians
inline float deg(float inDeg) { return inDeg * pi / 180.0f; }

// radians to degrees
inline float toDeg(float inRad) { return inRad * 180.0f / pi; }

/**
 * Convenient vector comparisions.
 */
template <class T>
bool operator ==(const vector<T> &a, const vector<T> &b) {
	if (a.size() != b.size())
		return false;
	for (nat i = 0; i < a.size(); i++)
		if (a[i] != b[i])
			return false;
	return true;
}

template <class T>
bool operator !=(const vector<T> &a, const vector<T> &b) {
	return !(a == b);
}

template <class T>
bool operator <(const vector<T> &a, const vector<T> &b) {
	nat to = min(a.size(), b.size());
	for (nat i = 0; i < to; i++) {
		if (a[i] != b[i])
			return a[i] < b[i];
	}

	return a.size() < b.size();
}

template <class From, class To, bool custom>
struct my_cast {
	inline To *cast(From *f) { return dynamic_cast<To *>(f); }
};

template <class From, class To>
struct my_cast<From, To, true> {
	inline To *cast(From *f) { return customAs<To>(f); }
};

#include "Detect.h"
CREATE_DETECTOR(isA);

template <class T>
struct as {
	template <class U>
	as(U *v) {
		my_cast<U, T, detect_isA<U>::value> c;
		this->v = c.cast(v);
		// this->v = dynamic_cast<T*>(v);
	}

	operator T*() const {
		return v;
	}

	T *v;
};

#include "Object.h"
#include "Debug.h"
#include "Printable.h"
