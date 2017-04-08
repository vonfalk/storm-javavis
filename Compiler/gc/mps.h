#pragma once

/**
 * The MPS gc. Only includes something if the MPS gc is chosen.
 */

#ifdef STORM_GC_MPS

#ifdef __cplusplus
extern "C" {
#endif

#if defined(SLOW_DEBUG)
	// Run MPS in its cool (as opposed to hot) configuration when in debug mode.
#define CONFIG_VAR_COOL
#else
	// Run MPS in the hot configuration.
#define CONFIG_VAR_COOL
#endif

#include "mps/code/mps.h" // MPS core
#include "mps/code/mpsavm.h" // VM arena
#include "mps/code/mpscamc.h" // AMC pool
#include "mps/code/mpscams.h" // AMS pool
#include "mps/code/mpscawl.h" // AWL pool
#include "mps/code/mpscmvff.h" // MVFF pool

	// Decrease/increase the # of byte scanned for this scanned set (hack).
	void mps_decrease_scanned(mps_ss_t ss, size_t decrease);
	void mps_increase_scanned(mps_ss_t ss, size_t increase);

#ifdef __cplusplus
}
#endif

#endif
