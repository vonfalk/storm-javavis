#include "stdafx.h"
#include "Shared/DllMain.h"
#include "Audio.h"

STORM_LIB_ENTRY_POINT(sound::LibData);

#pragma comment(lib, "dsound.lib")

namespace sound {

	LibData::LibData() {}

	LibData::~LibData() {}

	void LibData::shutdown() {
		if (audio)
			audio->terminate();
	}


	os::Thread spawnAudio(Engine &e) {
		return os::Thread::spawn(new AudioWait(e));
	}

	DEFINE_STORM_THREAD_WAIT(Audio, &spawnAudio);

}
