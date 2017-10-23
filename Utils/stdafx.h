// #pragma once // GCC issues a warning when using 'pragma once' with precompiled headers...
#ifndef UTILS_PCH
#define UTILS_PCH

// When compiling from C: do not include much at all, as most of the Utils.h is C++.
#ifdef __cplusplus

#include "targetver.h"

#include "Utils.h"

#endif
#endif
