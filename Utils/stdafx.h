#pragma once

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN            // Exclude rarely-used stuff from Windows headers
#endif

#if defined(_DEBUG) && !defined(DEBUG)
#define DEBUG
#endif

#include <stdio.h>
#include <tchar.h>
#include <windows.h>

#include "targetver.h"

#include "Utils.h"
