#pragma once
#include "Core/Io/Url.h"

namespace storm {

	/**
	 * System-specific details on how shared libraries are accessed.
	 */

	/**
	 * Type used to represent shared libraries on this platform.
	 */
#if defined(WINDOWS)
	typedef HMODULE LoadedLib;
#elif defined(POSIX)
	typedef void *LoadedLib;
#else
#error "Please implement shared libraries for your platform."
#endif

	// Invalid handle value.
	extern LoadedLib invalidLib;


	// Load a shared library. Returns 'invalidLib' on failure.
	LoadedLib loadLibrary(Url *file);

	// Unload a shared library.
	void unloadLibrary(LoadedLib lib);

	// Find a function in the loaded library. Returns null on failure.
	const void *findLibraryFn(LoadedLib lib, const char *name);

	// Examine if 'file' has the export 'name'. Might give false positives.
	bool hasExport(Url *file, const char *name);

}
