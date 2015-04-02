#pragma once
#include "Platform.h"

/**
 * Endianness stuff.
 */

// Swap the byte order of a value.
int16 byteSwap(int16 v);
nat16 byteSwap(nat16 v);
int byteSwap(int v);
nat byteSwap(nat v);

// Swap if needed to get to network order (big endian).
#ifdef LITTLE_ENDIAN
inline void networkSwap(int16 &v) { v = byteSwap(v); }
inline void networkSwap(nat16 &v) { v = byteSwap(v); }
inline void networkSwap(int &v) { v = byteSwap(v); }
inline void networkSwap(nat &v) { v = byteSwap(v); }
#else
inline void networkSwap(int16 &v) {}
inline void networkSwap(nat16 &v) {}
inline void networkSwap(int &v) {}
inline void networkSwap(nat &v) {}
#endif

