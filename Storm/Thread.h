#pragma once
#include "Shared/Thread.h"
#include "Shared/EnginePtr.h"

namespace storm {
	// TODO: Where should this be? Is core good?
	STORM_PKG(core);

	// This is the thread for the compiler.
	STORM_THREAD(Compiler);


	STORM_PKG(core.lang);

	class NamedThread;

	// Get the named thread for the compiler.
	NamedThread *STORM_ENGINE_FN compilerThread(EnginePtr e) ON(Compiler);

}
