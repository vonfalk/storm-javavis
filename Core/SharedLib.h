#pragma once
#include "EngineFwd.h"

namespace storm {
	struct CppWorld;

	/**
	 * Parameters to and from a shared lib during initialization.
	 */

	/**
	 * Parameters to the shared lib.
	 */
	struct SharedLibStart {
		// TODO: Include some kind of versioning scheme here, so that the libraries can detect when
		// incompatible versions are used!

		// Owning engine.
		Engine &engine;

		// Shared and unique function pointers.
		const EngineFwdShared &shared;
		const EngineFwdUnique &unique;
	};

	/**
	 * Information returned about a shared lib. This is allocated by the shared library, and freed
	 * using the function pointer returned inside.
	 */
	struct SharedLibInfo {
		// All types in the library.
		const CppWorld *world;

		// Function called to destroy this structure.
		typedef void (*DestroyFn)(SharedLibInfo *);
		DestroyFn destroyFn;
	};

}
