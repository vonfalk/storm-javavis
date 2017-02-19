#include "stdafx.h"
#include "Engine.h"
#include "Core/EngineFwd.h"
#include "Core/SharedLib.h"
#include "Core/Gen/CppTypes.h"

namespace storm {

	// Free a SharedLibInfo struct.
	static void freeInfo(SharedLibInfo *info) {
		delete info;
	}

	SharedLibInfo *sharedLibEntry(const SharedLibStart *params) {
		void *prev = params->engine.attach(params->shared, params->unique);

		SharedLibInfo *result = new SharedLibInfo;
		result->world = storm::cppWorld();
		result->previousIdentifier = prev;
		result->shutdownFn = null;
		result->destroyFn = &freeInfo;
		return result;
	}

}
