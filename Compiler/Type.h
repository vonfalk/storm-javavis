#pragma once
#include "Object.h"
#include "Gc.h"

namespace storm {

	/**
	 * Description of a type.
	 */
	class Type : public Object {
	public:
		// Create a type.
		Type();

		// Destroy our resources.
		~Type();

		// Owning engine.
		Engine &engine;

		// Create the type for Type (as this is special). Assumed to be run while the gc is parked.
		Type *createType(Engine &e);

	private:
		// Special constructor for creating the first type.
		Type(Engine &e);

		// The description of the type we maintain for the GC. Not valid if we're a value-type.
		// Note: care must be taken whenever this is manipulated!
		GcType *gcType;
	};

}
