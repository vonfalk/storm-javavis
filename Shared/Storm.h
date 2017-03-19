#pragma once
#include "Core/Storm.h"
#include "Core/Str.h"
#include "Core/StrBuf.h"


namespace storm {
	struct SharedLibInfo;
	struct SharedLibStart;
}

// Entry-point for the shared library. Needs to be declared in the project that actually produces
// the DLL, otherwise the compiler will miss exporting the function.
extern "C" SHARED_EXPORT bool SHARED_LIB_ENTRY(const storm::SharedLibStart *params, storm::SharedLibInfo *out);
