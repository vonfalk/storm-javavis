// #pragma once // GCC issues a warning when using 'pragma once' with precompiled headers...
#ifndef GC_PCH
#define GC_PCH

#include "Utils/Mode.h"
#include "Config.h"

// Some C-files include this header. We can't include other things then.
#ifdef __cplusplus
#include "Core/Storm.h"

// Note: We only need this for 'Code/Refs'. We would like to be a stand-alone library somewhere down the road.
#include "Code/Code.h"
#endif

#endif
