#include "stdafx.h"
#include "Engine.h"
#include "Core/EngineFwd.h"
#include "Core/SharedLib.h"
#include "Core/Gen/CppTypes.h"

using namespace storm;

/**
 * Free a SharedLibInfo struct.
 */
static void freeInfo(SharedLibInfo *info) {
	delete info;
}

/**
 * Entry-point for the shared library.
 */
extern "C"
SHARED_EXPORT SharedLibInfo *SHARED_LIB_ENTRY(const SharedLibStart *params) {
	params->engine.attach(params->shared, params->unique);

	SharedLibInfo *result = new SharedLibInfo;
	result->world = storm::cppWorld();
	result->destroyFn = &freeInfo;
	return result;
}
