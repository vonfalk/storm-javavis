#pragma once

#include "Utils/Utils.h"
#include "Shared/Storm.h"

#include "Utils/Windows.h"

#include "Shared/DllEngine.h"

using namespace storm;

namespace sound {
	class AudioMgr;

	/**
	 * Global data.
	 */
	class LibData : public storm::LibData {
	public:
		LibData();
		~LibData();

		// Shutdown.
		void shutdown();


		// AudioMgr object.
		Auto<sound::AudioMgr> audio;
	};

	// Audio thread.
	STORM_THREAD(Audio);

    /**
	 * Error.
	 */
	class SoundOpenError : public Exception {
	public:
		SoundOpenError(const String &msg) : msg(msg) {}
		virtual String what() const { return L"Error while loading sound: " + msg; }
	private:
		String msg;
	};

}

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

#include "Shared/Io/Stream.h"
namespace sound {
	// Avoid name collisions.
	using storm::IStream;
}
