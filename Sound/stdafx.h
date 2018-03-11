// #pragma once // GCC issues a warning when using 'pragma once' with precompiled headers...
#ifndef SOUND_H
#define SOUND_H
#include "Shared/Storm.h"
#include "Utils/Platform.h"
#include "Core/Io/Stream.h"

namespace sound {
	using namespace storm;
	using storm::IStream;

	/**
	 * Thread used for audio processing. All classes that provides sound are invoked from this thread.
	 */
	STORM_THREAD(Audio);
}

/**
 * Which backend do we use?
 *
 * Default is:
 * DirectSound on Windows (SOUND_DX)
 * OpenAL on others (SOUND_AL)
 */
#ifdef WINDOWS
#define SOUND_DX
#else
#define SOUND_AL
#endif


#ifdef SOUND_DX

// Use DirectSound on Windows.
#include <mmreg.h> // Ignored when using WIN32_LEAN_AND_MEAN
#include <initguid.h>
#include <ks.h> // GUID_NULL is defined here and not included with 'initguid'...
#include <dsound.h>

#undef interface

// Release COM objects.
template <class T>
void release(T *&v) {
	if (v)
		v->Release();
	v = null;
}

#endif

#ifdef SOUND_AL

// Use OpenAl on other operating systems.
#include <AL/al.h>
#include <AL/alc.h>

#endif

// For flac...
#define FLAC__NO_DLL

#endif
