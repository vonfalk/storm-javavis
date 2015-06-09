#pragma once
#ifdef STORM_DLL
#include "DllInterface.h"

namespace storm {

	// This should be defined before 'DllMain.h' is included.
	class LibData;

    /**
     * Implementation of the Engine type for DLL:s. This type can not contain any data, and will only
     * pass through calls to the owning Engine. This Engine is hereby referred to as a slave Engine.
     */
	class Engine {
	public:
		// Set up the engine for this interface. The interface is the same for all Engines
		// within the same process, so we store the interface in a global variable.
		static DllInfo setup(const DllInterface *interface);

		// Get a built in type.
		Type *builtIn(nat id);

		// Get the data.
		LibData *data();

		// Get a specific thread object.
		Thread *thread(uintptr_t id);

	};

	// Get the C++ vtable for a type. Used for external types.
	void *cppVTable(nat id);

}

#endif
