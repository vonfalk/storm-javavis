#pragma once
#include "Shared/Auto.h"

namespace storm {
#ifdef WINDOWS
	typedef HMODULE DynamicLib;
#else
#error "Please implement dynamic libraries for your platform here."
#endif

	class Url;
	struct BuiltIn;

	/**
	 * Keeps track of dynamically loaded libraries, and makes sure we unload them when we do not
	 * need them anymore (currently only at shutdown).
	 */
	class DynamicLibs : NoCopy {
	public:
		DynamicLibs();
		~DynamicLibs();

		// Load a library, set it up and return the information from it.
		const BuiltIn *load(Par<Url> url);

	private:
		// Loaded libraries.
		vector<DynamicLib> loaded;
	};

}
