// #pragma once // GCC issues a warning when using 'pragma once' with precompiled headers...
#ifndef GC_PCH
#define GC_PCH

#include "Utils/Mode.h"
#include "Config.h"

// Some C-files include this header. We can't include other things then.
#ifdef __cplusplus
#include "Core/Storm.h"
#endif

#endif
