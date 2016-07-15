#pragma once
#include "Named.h"
#include "Gc.h"
#include "TypeFlags.h"

namespace storm {

	// Description of a type in C++. Found in CppTypes.h
	struct CppType;

	/**
	 * Description of a type.
	 */
	class Type : public Named {
		STORM_CLASS;

		// Let allocation access gcType.
		friend void *allocObject(size_t s, Type *t);

	public:
		// Create a type declared in Storm.
		STORM_CTOR Type(Str *name, TypeFlags flags);

		// Create a type declared in C++.
		STORM_CTOR Type(Str *name, TypeFlags flags, Size size, GcType *gcType);

		// Destroy our resources.
		~Type();

		// Owning engine.
		Engine &engine;

		// Create the type for Type (as this is special). Assumed to be run while the gc is parked.
		static Type *createType(Engine &e, const CppType *type);

	private:
		// Special constructor for creating the first type.
		Type(Engine &e, TypeFlags flags, Size size, GcType *gcType);

		// The description of the type we maintain for the GC. Not valid if we're a value-type.
		// Note: care must be taken whenever this is manipulated!
		GcType *gcType;


		// Special case for the first Type.
		static void *operator new(size_t size, Engine &e, GcType *type);
		static void operator delete(void *mem, Engine &e, GcType *type);
	};


}
