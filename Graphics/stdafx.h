#pragma once

#include "Utils/Utils.h"
#include "Shared/Storm.h"
#include "Utils/Windows.h"

namespace graphics {
	using namespace storm;
}

#include "Shared/DllEngine.h"
#include "Shared/Graphics.h"

// Release COM objects.
template <class T>
void release(T *&v) {
	if (v)
		v->Release();
	v = null;
}
