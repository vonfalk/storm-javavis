#pragma once

/**
 * Include this file from a file in the shared library, and use the macro
 * SHARED_LIB_ENTRY_POINT() to declare de shared library's entry point.
 */

#include "Engine.h"
#include "Core/EngineFwd.h"
#include "Core/SharedLib.h"
#include "Core/Gen/CppTypes.h"

#define SHARED_LIB_ENTRY_POINT()										\
	extern "C" SHARED_EXPORT bool SHARED_LIB_ENTRY(const storm::SharedLibStart *params, storm::SharedLibInfo *info) { \
		return storm::sharedLibEntry(*params, *info);					\
	}


namespace storm {

	/**
	 * Entry-point for the shared library.
	 */
	bool sharedLibEntry(const SharedLibStart &params, SharedLibInfo &out);

}
