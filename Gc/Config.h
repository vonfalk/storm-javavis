#pragma once

/**
 * Select the GC to use here.
 *
 * ZERO is just allocating memory without ever returning it. Usable as a template for new GC
 * implementations. Note: Storm will not shut down properly unless destructors are called, which the
 * ZERO gc does not do.
 *
 * MPS is currently the fastest GC supported, but requires all programs using it to make the source
 * code available (or to acquire another license for the MPS).
 */

#ifdef DEBUG
// Slow but better debugging?
//#define SLOW_DEBUG
#endif

// Constants for selecting a GC.
#define STORM_GC_ZERO 0
#define STORM_GC_MPS 1
// ...

// Select the GC to use.
#define STORM_GC STORM_GC_MPS

