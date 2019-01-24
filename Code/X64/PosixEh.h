#pragma once
#include "Utils/Platform.h"
#include "DwarfEh.h"
#ifdef POSIX
#include <unwind.h>
#endif

namespace code {
	namespace x64 {
#ifdef POSIX

		/**
		 * Implementation of the runtime required to properly use the exceptions generated in
		 * DwarfEh.h. Currently specific to POSIX-like systems.
		 */

		// Personality function for Storm.
		_Unwind_Reason_Code stormPersonality(int version, _Unwind_Action actions, _Unwind_Exception_Class type,
											_Unwind_Exception *data, _Unwind_Context *context);
#else

		// We need to define a dummy personality function so that other platforms link properly.
		void stormPersonality();

#endif

		// Activate stack info in stack traces.
		void activateInfo();

	}
}
