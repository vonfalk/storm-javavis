#pragma once

/**
 * Storm supports a few garbage collectors. Choose one here:
 *
 * TODO: Make it possible to choose as a parameter to mymake.
 *
 * MPS is the fastest and most flexible, but requires all programs using it to make the source code
 * available (or to acquire another license for the MPS).
 *
 * TODO: Add support for boehm GC as well: http://hboehm.info/gc/ (very liberal license, not as powerful).
 */

// C-mode detection of debug mode.
#include "Utils/Mode.h"

#define STORM_GC_MPS
// #define STORM_GC_BOEHM

// When compiling in C-mode, we do not want to include these:
#ifdef __cplusplus

#include "Utils/Utils.h"
#include "Utils/Platform.h"
#include "Code/Code.h"

namespace storm {
	using code::Size;
	using code::Offset;

	class Engine;
}

#endif
