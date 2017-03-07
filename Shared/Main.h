#pragma once

/**
 * Include this file from a file in the shared library, and use the macro
 * SHARED_LIB_ENTRY_POINT(<data>) to declare de shared library's entry point.
 */

#include "Engine.h"
#include "Core/EngineFwd.h"
#include "Core/SharedLib.h"
#include "Core/Gen/CppTypes.h"

#define SHARED_LIB_ENTRY_POINT(data)									\
	extern "C" SHARED_EXPORT void SHARED_LIB_ENTRY(const storm::SharedLibStart *params, storm::SharedLibInfo *info) { \
		*info = storm::sharedLibEntry(params);							\
	}


namespace storm {

	/**
	 * Entry-point for the shared library.
	 */
	SharedLibInfo sharedLibEntry(const SharedLibStart *params);

}
