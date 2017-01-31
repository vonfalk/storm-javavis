#pragma once
#include "Core/Str.h"
#include "Core/EnginePtr.h"
#include "Core/Io/Text.h"

namespace storm {
	namespace io {
		STORM_PKG(core);

		/**
		 * Standard IO for use from Storm.
		 */

		// Quick print line function.
		void STORM_FN print(Str *s) ON(Compiler);

		// Get the stdout text stream.
		TextInput *STORM_FN stdIn(EnginePtr e) ON(Compiler);
		TextOutput *STORM_FN stdOut(EnginePtr e) ON(Compiler);
		TextOutput *STORM_FN stdError(EnginePtr e) ON(Compiler);

	}
}
