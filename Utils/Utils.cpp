// stdafx.cpp : source file that includes just the standard includes
// Spel.pch will be the pre-compiled header
// stdafx.obj will contain the pre-compiled type information

#include "stdafx.h"

bool getAsyncKeyState(nat key) { //Hur är det just nu?
	return (GetAsyncKeyState(key) & 0x8000) != 0;
}

bool getKeyState(nat key) { //Hur var det när nuv. meddelande skickades?
	return (GetKeyState(key) & 0x8000) != 0;
}


bool positive(float f) {
	return f >= 0.0f;
}


#ifdef WIN32

// Check alignment of value.
static inline bool aligned(volatile void *v) {
	UINT_PTR i = (UINT_PTR)v;
	return (i & 0x3) == 0;
}

nat atomicIncrement(volatile nat &v) {
	assert(aligned(&v));
	return (nat)InterlockedIncrement((volatile LONG *)&v);
}

nat atomicDecrement(volatile nat &v) {
	assert(aligned(&v));
	return (nat)InterlockedDecrement((volatile LONG *)&v);
}

#else
#error "atomicIncrement and atomicDecrement are only supported on Windows for now"
#endif
