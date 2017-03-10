#pragma once

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
