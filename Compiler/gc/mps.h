#pragma once

/**
 * The MPS gc. Only includes something if the MPS gc is chosen.
 */

#ifdef STORM_GC_MPS

#ifdef __cplusplus
extern "C" {
#endif

#ifdef DEBUG
	// Run MPS in its cool (as opposed to hot) configuration when in debug mode.
#define CONFIG_VAR_COOL
#endif

#include "mps/code/mps.h" // MPS core
#include "mps/code/mpsavm.h" // VM arena
#include "mps/code/mpscamc.h" // AMC pool

	// Decrease the # of byte scanned for this scanned set (hack).
	void mps_decrease_scanned(mps_ss_t ss, size_t decrease);

#ifdef __cplusplus
}
#endif

#endif
