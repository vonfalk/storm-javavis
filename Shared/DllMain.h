#pragma once

/**
 * Include this file _once_ from any extension DLL to storm. This file declares the entry points
 * that storm expects to find.
 *
 * We assume that a class named 'LibData' exists, which can contain any static objects required
 * by the DLL. One 'LibData' is allocated per engine that loads the dll.
 */


#include "Utils/Platform.h"
#include "DllEngine.h"
#include "DllInterface.h"

#ifdef WINDOWS

#define DLLEXPORT __declspec(dllexport)

// This is the entry point Storm uses to get everything contained in this package.
extern "C" DLLEXPORT storm::DllInfo ENTRY_POINT_NAME(const storm::DllInterface *interface) {
	return storm::Engine::setup(interface);
}


#else
#error "Building DLLS is not supported for your platform yet."
#endif

// Needed source files:
#include "DllEngine.cpp"
