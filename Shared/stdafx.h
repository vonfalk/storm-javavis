// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers

#include "Utils/Utils.h"

// Indicate we're being compiled as the library.
#define STORM_LIB
#include "Storm.h"
