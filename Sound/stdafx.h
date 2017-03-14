#pragma once
#include "Shared/Storm.h"
#include "Core/Io/Stream.h"

namespace sound {
	using namespace storm;
	using storm::IStream;

	STORM_THREAD(Audio);
}


#include <mmreg.h> // Ignored when using WIN32_LEAN_AND_MEAN
#include <initguid.h>
#include <ks.h> // GUID_NULL is defined here and not included with 'initguid'...
#include <dsound.h>

// For flac...
#define FLAC__NO_DLL

#undef interface

// Release COM objects.
template <class T>
void release(T *&v) {
	if (v)
		v->Release();
	v = null;
}
