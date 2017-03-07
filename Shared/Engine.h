#pragma once
#include "Core/EngineFwd.h"

namespace storm {

	/**
	 * Implements the engine used in shared libraries. An engine object in the compiler is different
	 * (even by pointer) from engine objects in any shared libraries. This is since the compiler
	 * needs to keep track of the different type id:s found in different libraries.
	 */
	class Engine : NoCopy {
	private:
		// The ID of this engine. This must be the first member, as it has to match up with the ID
		// in Compiler/Engine.h.
		Nat id;
	public:
		// Attach this engine to the current shared library. Returns the identifier from any previous instance.
		void *attach(const EngineFwdShared &shared, const EngineFwdUnique &unique);

		// Attach one engine from the current shared library.
		void detach();

		// Get the data for our library.
		void *data();

		// Get the id.
		inline Nat identifier() const { return id; }
	};

}
