#pragma once
#ifdef STORM_DLL
#include "DllInterface.h"

namespace storm {

	/**
	 * Base class for library-specific data.
	 */
	class LibData : public NoCopy {
	public:
		// Ctor.
		LibData();

		// Dtor. Called when the Engine is unloading this library. At this point, the type system
		// may not still be intact. Any shutdown that require interaction with the type system, or
		// that may spawn thread should be done in 'shutdown'.
		virtual ~LibData();

		// Called when the Engine starts to shutdown, before types are being elliminated.  It is
		// common to respond to this message by attempting to shut down everything that potentially
		// runs code outside of the DLL, or requires any interaction with the type-system.
		virtual void shutdown();
	};


    /**
     * Implementation of the Engine type for DLL:s. This type can not contain any data, and will only
     * pass through calls to the owning Engine. This Engine is hereby referred to as a slave Engine.
     */
	class Engine {
	public:
		// Set up the engine for this interface. The interface is the same for all Engines
		// within the same process, so we store the interface in a global variable.
		static DllInfo setup(const DllInterface *interface, LibData *data);

		// Get a built in type.
		Type *builtIn(nat id);

		// Get the data.
		LibData *data();

		// Get a specific thread object.
		Thread *thread(uintptr_t id, DeclThread::CreateFn fn);
	};

	// Get the C++ vtable for a type. Used for external types.
	void *cppVTable(nat id);

}

#endif
