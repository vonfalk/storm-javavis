#pragma once
#include "Core/Thread.h"
#include "Core/TObject.h"

namespace storm {
	STORM_PKG(core);

	/**
	 * Declare the compiler thread, where the compiler itself is to run.
	 */
	STORM_THREAD(Compiler);

}
