// #pragma once // GCC issues a warning when using 'pragma once' with precompiled headers...
#ifndef GRAPHICS_H
#define GRAPHICS_H

#include "Utils/Utils.h"
#include "Shared/Storm.h"
#include "Utils/Win32.h"

namespace graphics {
	using namespace storm;
}

// Release COM objects.
template <class T>
void release(T *&v) {
	if (v)
		v->Release();
	v = null;
}

#endif
