#include "stdafx.h"
#include "Shared/Main.h"

#pragma comment(lib, "dsound.lib")

SHARED_LIB_ENTRY_POINT();

namespace sound {
	os::Thread spawnAudioThread(Engine &e);
	STORM_DEFINE_THREAD_WAIT(Audio, &spawnAudioThread);
}
