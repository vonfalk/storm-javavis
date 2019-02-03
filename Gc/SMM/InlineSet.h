#pragma once

#if STORM_GC == STORM_GC_SMM

#include "OS/InlineSet.h"

namespace storm {
	namespace smm {

		/**
		 * We use the InlineSet in the OS/ library. That provides the ability to iterate through the
		 * set at any point in time, even during a modification.
		 */
		using os::InlineSet;
		using os::SetMember;

	}
}

#endif
