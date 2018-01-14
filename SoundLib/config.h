#pragma once

// On Windows, we need the header from the MSVC++ port.
#ifdef _MSC_VER
#define WIN32
#define strcasecmp _strcmpi
#define strncasecmp _strnicmp

#include "mpg123/ports/MSVC++/mpg123.h"

// uint16_t are missing from the above header...
#if _MSC_VER < 1600
typedef __int16 int16_t;
typedef unsigned __int16 uint16_t;
#endif

#endif

