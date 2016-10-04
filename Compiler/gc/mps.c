#include "stdafx.h"
#include "mps.h"

#ifdef STORM_GC_MPS

/**
 * Note: Defining CONFIG_VAR_COOL or CONFIG_VAR_RASH from mymake will compile the 'cool' or the
 * 'rash' variety of MPS as opposed to the 'hot' variety. The cool variety will do more checking on
 * the hot path of MPS, while the rash variety does no checking and contains no telemetry output.
 */

#include "mps/code/mps.c" // Includes all code needed for MPS.

// Helper function that abuses the internals of MPS a bit.
void mps_decrease_scanned(mps_ss_t mps_ss, size_t decrease) {
	ScanState ss = PARENT(ScanStateStruct, ss_s, mps_ss);
	ss->scannedSize -= decrease;
}

void mps_increase_scanned(mps_ss_t mps_ss, size_t increase) {
	ScanState ss = PARENT(ScanStateStruct, ss_s, mps_ss);
	ss->scannedSize += increase;
}

// Custom abort which helps when debugging.
void mps_custom_abort() {
#ifdef DEBUG
	DebugBreak();
#endif
	abort();
}

#endif
