#pragma once
#include "Utils/Platform.h"

#if defined(WINDOWS) && defined(X86)


namespace code {
	namespace x86 {

		/**
		 * Windows-specific exception-handling.
		 */


		// Activate stack traces.
		void activateInfo();

	}
}

#endif
