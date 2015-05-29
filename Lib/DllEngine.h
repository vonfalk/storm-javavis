#pragma once
#ifdef STORM_DLL
#include "DllInterface.h"

namespace storm {

    /**
     * Implementation of the Engine type for DLL:s. This type does not contain any data, and will only
     * pass through calls to the owning Engine. This Engine is hereby referred to as a slave Engine.
     */
	class Engine {
	public:
		// Set up the engine for this interface. The interface is the same for all Engines
		// within the same process, so we store the interface in a global variable.
		static void setup(const DllInterface *interface);

		Type *builtIn(nat id);
	};

}

#endif
