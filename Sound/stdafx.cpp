#include "stdafx.h"
#include "Shared/DllMain.h"
#include "Audio.h"

#pragma comment(lib, "dsound.lib")

namespace storm {

	LibData::LibData() {}

	LibData::~LibData() {
		if (audio)
			audio->terminate();
	}

}


namespace sound {

	os::Thread spawnAudio(Engine &e) {
		return os::Thread::spawn(new AudioWait(e));
	}

	DEFINE_STORM_THREAD_WAIT(Audio, &spawnAudio);

}
