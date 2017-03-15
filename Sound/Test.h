#pragma once
#include "Core/EnginePtr.h"

namespace sound {

	// Simple test function which performs playback without requiring Basic Storm.
	void STORM_FN testMe(EnginePtr e) ON(Audio);

}
