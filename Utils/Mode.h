#pragma once

// Preprocessor only detection of our current mode of operation. Has to be compilable in C-mode.
#if defined(_DEBUG) && !defined(DEBUG)
#define DEBUG
#endif

// Debug mode compiled in release mode!
#if defined(FAST_DEBUG) && !defined(DEBUG)
#define DEBUG
#endif

#ifdef DEBUG
// We don't need iterator debugging, it's slow!
#define _HAS_ITERATOR_DEBUGGING 0
#endif

