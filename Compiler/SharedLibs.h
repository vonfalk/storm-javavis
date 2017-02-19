#pragma once
#include "SharedLib.h"

namespace storm {

	/**
	 * Stores all shared libraries currently loaded by an engine.
	 */
	class SharedLibs : NoCopy {
	public:
		// Create.
		SharedLibs();

		// Destroy.
		~SharedLibs();

		// Attempt to load an URL with a shared library. Handles any duplications in the system.
		SharedLib *load(Url *file);

		// Signal that shutdown is in progress to all libraries.
		void shutdown();

		// Unload all libraries.
		void unload();

	private:
		// All loaded shared libraries so far.
		vector<SharedLib *> loaded;
	};

}
