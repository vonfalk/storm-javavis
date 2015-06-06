#pragma once

/**
 * Include this file _once_ from any extension DLL to storm. This file declares the entry points
 * that storm expects to find.
 */


#include "Utils/Platform.h"
#include "DllEngine.h"
#include "DllInterface.h"

#ifdef WINDOWS

#define DLLEXPORT __declspec(dllexport)

namespace storm {
	// Found in the BuiltIn cpp-file.
	struct BuiltIn;
	const BuiltIn &builtIn();
}

// This is the entry point Storm uses to get everything contained in this package.
extern "C" DLLEXPORT const storm::BuiltIn *ENTRY_POINT_NAME(const storm::DllInterface *interface) {
	storm::Engine::setup(interface);
	return &storm::builtIn();
}


#else
#error "Building DLLS is not supported for your platform yet."
#endif

// Needed source files:
#include "DllEngine.cpp"
