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

		// Basic version difference check: sizes of important structs.
		size_t sizeLibStart;
		size_t sizeLibInfo;
		size_t sizeFwdShared;
		size_t sizeFwdUnique;

		// Owning engine.
		Engine &engine;

		// Shared and unique function pointers.
		const EngineFwdShared &shared;
		const EngineFwdUnique &unique;
	};

	/**
	 * Information returned about a shared lib. May only contain pointers, as it is sometimes
	 * allocated in a GC root.
	 */
	struct SharedLibInfo {
		// All types in the library.
		const CppWorld *world;

		// The 'identifier' parameter from 'unique' in any previous instance.
		void *previousIdentifier;

		// Any data the library wishes to access.
		void *libData;

		typedef void (*CallbackFn)(SharedLibInfo *);

		// Function called to notify shutdown for this structure.
		CallbackFn shutdownFn;
	};

}
