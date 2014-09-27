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

void atomicIncrement(volatile nat &v) {
	InterlockedIncrement((volatile LONG *)&v);
}

void atomicDecrement(volatile nat &v) {
	InterlockedDecrement((volatile LONG *)&v);
}

#else
#error "atomicIncrement and atomicDecrement are only supported on Windows"
#endif
