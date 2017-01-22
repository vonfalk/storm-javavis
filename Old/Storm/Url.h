#pragma once
#include "Shared/EnginePtr.h"
#include "Shared/Io/Url.h"

namespace storm {
	STORM_PKG(core.io);

	// Get the url of the root package.
	Url *STORM_ENGINE_FN rootUrl(EnginePtr e);

}
