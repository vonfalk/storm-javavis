#pragma once
#include "Core/EnginePtr.h"
#include "Core/Io/Stream.h"

namespace storm {
	namespace server {
		STORM_PKG(core.lang.server);

		/**
		 * Run this compiler as a language server.
		 */
		void STORM_FN run(EnginePtr e, IStream *input, OStream *output);

	}
}
