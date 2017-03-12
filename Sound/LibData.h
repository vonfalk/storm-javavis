#pragma once
#include "Shared/LibData.h"

namespace sound {
	class AudioMgr;

	// Access the values in the library data.
	AudioMgr *&audioMgrData(Engine &e);

}
