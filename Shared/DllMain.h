#pragma once

/**
 * Include this file _once_ from any extension DLL to storm. This file declares the entry points
 * that storm expects to find.
 *
 * Also make sure to add STORM_LIB_ENTRY_POINT after including this file. This macro takes as a
 * parameter the library-specific class this library uses to hold any per-engine state inside.
 *
 * We assume that a class named 'LibData' exists, which can contain any static objects required
 * by the DLL. One 'LibData' is allocated per engine that loads the dll.
 */


#include "Utils/Platform.h"
#include "DllEngine.h"
#include "DllInterface.h"
#include "Storm.h"

#ifdef WINDOWS

#define DLLEXPORT __declspec(dllexport)

// This is the entry point Storm uses to get everything contained in this package.
#define STORM_LIB_ENTRY_POINT(libdata)									\
	extern "C" DLLEXPORT storm::DllInfo ENTRY_POINT_NAME(const storm::DllInterface *interface) { \
		return storm::Engine::setup(interface, new libdata());			\
	}


#else
#error "Building DLLS is not supported for your platform yet."
#endif

// Needed source files:
#include "DllEngine.cpp"
