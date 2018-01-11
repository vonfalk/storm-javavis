#include "stdafx.h"
#include "Utils/Mode.h"
#include "Utils/Platform.h"
#include "mps.h"

#ifdef STORM_GC_MPS

#pragma warning(disable: 4068) // Do not warn about unknown pragmas.
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"

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

// Custom assertion failure handler. Calls 'DebugBreak' to aid when debugging.
void mps_assert_fail(const char *file, unsigned line, const char *condition) {
	fflush(stdout);
	fprintf(stderr,
			"The MPS detected a problem!\n"
			"%s:%u: MPS ASSERTION FAILED: %s\n",
			file, line, condition);
	fflush(stderr);
	mps_telemetry_flush();
#ifdef DEBUG
#ifdef WINDOWS
	DebugBreak();
#else
	raise(SIGINT);
#endif
#endif
	abort();
}

#ifdef POSIX
void mps_on_sigsegv(int signal) {
	(void)signal;
	// Raise SIGINT instead. See the comment below for details.
	raise(SIGINT);
}
#endif

void mps_init() {
	// Custom assertions.
	mps_lib_assert_fail_install(&mps_assert_fail);

#ifdef DEBUG

#ifdef POSIX
	// Set up a handler for SIGSEGV that raises a SIGINT instead. This makes debugging in GDB a lot
	// easier, since it is not possible to distinguish between a genuine SIGSEGV and a SIGSEGV that
	// MPS handles otherwise.
	struct sigaction sa;
	sa.sa_handler = &mps_on_sigsegv;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;

	sigaction(SIGSEGV, &sa, NULL);
#endif

#endif
}

#endif
