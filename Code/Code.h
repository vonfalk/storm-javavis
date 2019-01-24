#pragma once
#include "Utils/Utils.h"
#include "Utils/Platform.h"
#include "OS/Types.h"
#include "Core/Storm.h"
#include "Gc/Config.h"

namespace code {

	// Use some types from Storm here as well.
	using namespace storm;
}

/**
 * Configuration of the code generation:
 */

// For X86: generate code that requires SSE3 (introduced in the P4 in 2004).
#define X86_REQUIRE_SSE3


