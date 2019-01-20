#pragma once

/**
 * Select the GC to use here.
 *
 * MPS is currently the fastest GC supported, but requires all programs using it to make the source
 * code available (or to acquire another license for the MPS).
 *
 * MALLOC is just using malloc() for allocating memory, but never returns it.
 */

#ifdef DEBUG
// Slow but better debugging?
//#define SLOW_DEBUG
#endif

// Use MPS?
#define STORM_GC_MPS
// Use MALLOC?
//#define STORM_GC_MALLOC
