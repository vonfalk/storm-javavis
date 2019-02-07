#pragma once

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN // Exclude rarely-used stuff from Windows headers.
#endif

// Use standalone stack walking (ie no external libraries). This will not
// always work well for optimized code. On Windows, the default is to use the DbgHelp library.
// #define STANDALONE_STACKWALK

#define _WIDEN(X) L ## X
#define WIDEN(X) _WIDEN(X)

#define _STRING(X) L ## #X
#define STRING(X) _STRING(X)

#define _SHORT_STRING(X) #X
#define SHORT_STRING(X) _SHORT_STRING(X)

#define UNUSED(x) (void)(x)

#include "Platform.h"
#include "Mode.h"

#define ARRAY_COUNT(x) (sizeof(x) / sizeof(*(x)))

#include <stdio.h>
#include <stddef.h>
#include <string.h>

#ifdef WINDOWS
#include <tchar.h>
#include "targetver.h"
#endif

#define _USE_MATH_DEFINES
#include <cmath>

// Include 'windows.h' if needed.
#include "Win32.h"

#include <algorithm>

const float pi = float(M_PI);

using std::min;
using std::max;
using std::swap;
using std::make_pair;

#ifdef VISUAL_STUDIO
typedef unsigned int nat;
typedef unsigned short nat16;
typedef short int16;
typedef unsigned __int64 uint64;
typedef unsigned __int64 nat64;
typedef __int64 int64;
typedef unsigned char byte;
typedef wchar_t wchar;
#else
#include <cstdint>
typedef uint32_t nat;
typedef uint16_t nat16;
typedef int16_t int16;
typedef uint64_t uint64;
typedef uint64_t nat64;
typedef int64_t int64;
typedef unsigned char byte;
typedef char16_t wchar;
#endif

#include "Containers.h"

#ifdef WINDOWS
inline int64 abs(int64 v) {
	return v < 0 ? -v : v;
}
#endif

#ifdef X64
// Allow max() and min() on Nat and size_t, even if they are different sizes.
inline size_t max(size_t x, nat y) { return max(x, size_t(y)); }
inline size_t max(nat x, size_t y) { return max(size_t(x), y); }
inline size_t min(size_t x, nat y) { return min(x, size_t(y)); }
inline size_t min(nat x, size_t y) { return min(size_t(x), y); }
#endif

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

template <class T>
inline T clamp(const T &v, const T &minV, const T &maxV) {
	return min(maxV, min(minV, v));
}

#ifdef POSIX
#define null nullptr
#else
#define null NULL
#endif

inline float degToRad(float angle) {
	return float(angle * M_PI / 180.0);
}

inline float radToDeg(float angle) {
	return float(angle * 180.0 / M_PI);
}


// Hack to allow casting member function pointers into void *.
template <class T>
inline const void *address(T fn) {
#ifdef VISUAL_STUDIO
	return (const void *&)fn;
#else
	return reinterpret_cast<const void *>(fn);
#endif
}

// Hack to allow casting void * into member function pointers.
template <class Fn>
inline Fn asMemberPtr(const void *fn) {
	union {
		Fn fn;
		const void *raw;
	} x;
	memset(&x, 0, sizeof(x));
	x.raw = fn;
	return x.fn;
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
	for (typename T::iterator i = vec.begin(), end = vec.end(); i != end; ++i)
		delete *i;
	vec.clear();
}

// Clear the contents of a map<?, ?*>.
template <class T>
inline void clearMap(T &map) {
	for (typename T::iterator i = map.begin(), end = map.end(); i != end; ++i)
		delete i->second;
	map.clear();
}

//Disable warning about using this pointer in ctor of objects since
//this occurs in almost every class using the member function pointer.
#pragma warning ( disable : 4355 )

// Ignore unknown pragmas.
#pragma warning ( disable : 4068 )


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

// Copy an array.
template <class T>
inline void copyArray(T *to, const T *from, nat count) {
	memcpy(to, from, count * sizeof(T));
}

const float defTolerance = 0.01f;
inline float eq(float a, float b, float tolerance = defTolerance) {
	return a + tolerance >= b && a - tolerance <= b;
}

// Atomic increase and decrease of variables
size_t atomicIncrement(volatile size_t &v);
size_t atomicDecrement(volatile size_t &v);

// Atomic bitwise operations.
size_t atomicAnd(volatile size_t &v, size_t with);
size_t atomicOr(volatile size_t &v, size_t with);
size_t atomicXor(volatile size_t &v, size_t with);

// Compare and swap (atomic)
size_t atomicCAS(volatile size_t &v, size_t compare, size_t exchange);
void *atomicCAS(void *volatile &v, void *compare, void *exchange);
template <class T>
T *atomicCAS(T *volatile &v, T *compare, T *exchange) {
	return (T *)atomicCAS((void *volatile&)v, compare, exchange);
}

// Atomic read/write. Also prevents the compiler from reordering memory accesses around the call.
size_t atomicRead(volatile size_t &v);
void *atomicRead(void *volatile &v);
void atomicWrite(volatile size_t &v, size_t value);
void atomicWrite(void *volatile &v, void * value);

template <class T>
void atomicWrite(T *volatile &v, T *value) {
	atomicWrite((void *volatile&)v, value);
}
template <class T>
T *atomicRead(T *volatile &v) {
	return (T *)atomicRead((void *volatile&)v);
}

// Atomic read/write. These two does not need aligned data.
size_t unalignedAtomicRead(volatile size_t &v);
void unalignedAtomicWrite(volatile size_t &v, size_t value);
void shortUnalignedAtomicWrite(volatile nat &v, nat value);

#ifdef X64
nat atomicIncrement(volatile nat &v);
nat atomicDecrement(volatile nat &v);
nat atomicCAS(volatile nat &v, nat compare, nat exchange);
nat atomicRead(volatile nat &v);
void atomicWrite(volatile nat &v, nat value);
#endif

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

inline bool all(const vector<bool> &v) {
	for (nat i = 0; i < v.size(); i++)
		if (!v[i])
			return false;
	return true;
}


namespace cast_impl {
	// Use the DynamicCast member if it exists, otherwise fall back to dynamic_cast.

	template <class To, class From>
	To *cast(From *from, typename From::DynamicCast *) {
		// TODO: It would be neat to enforce that either both 'From' and 'To' are const, or none of them.
		return From::DynamicCast::template cast<To>(from);
	}

	template <class To, class From>
	To *cast(From *from, ...) {
		return dynamic_cast<To *>(from);
	}
}


/**
 * Custom typecasting using as<T>(foo) instead of dynamic_cast<T *>(foo).
 *
 * Uses custom casting for the source type if it contains a type 'DynamicCast'. This type should
 * contain a function 'cast<T>()' that performs the typecasting.
 */
template <class To>
class as {
public:
	template <class From>
	as(From *v) : v(cast_impl::cast<To, From>(v, 0)) {}

	operator To*() const {
		return v;
	}

	To *operator ->() const {
		return v;
	}

private:
	To *v;
};

#include "Object.h"
#include "Debug.h"
#include "Assert.h"

template <class T>
void dumpHex(const T &v) {
	PLN(toHex(&v, sizeof(T)));
}

// Use common controls.
#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

// Some other libraries we need on Windows.
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "ws2_32.lib")
