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

#ifdef __cplusplus

// When compiling in C-mode, we do not want to include these:
#include "Core/Storm.h"
#include "Code/Code.h"

namespace storm {

	using code::Size;
	using code::Offset;

}

#endif
