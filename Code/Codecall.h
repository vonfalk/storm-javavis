#pragma once
#include "Utils/Platform.h"

#ifdef WINDOWS
#define CODECALL __cdecl
#endif

// Make sure everything is defined.
#ifndef CODECALL
#error "Someone forgot to declare CODECALL for your architecture."
#endif
