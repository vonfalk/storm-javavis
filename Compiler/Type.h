#pragma once
#include "Object.h"
#include "Gc.h"

namespace storm {

	// Description of a type in C++. Found in CppTypes.h
	struct CppType;

	/**
	 * Define different properties for a type.
	 */
	enum TypeFlags {
		// Regular type.
		typeClass = 0x00,

		// Value type?
		typeValue = 0x01,
	};

	/**
	 * Description of a type.
	 */
	class Type : public Object {
		STORM_CLASS;

		// Let object access gcType.
		friend void *Object::operator new(size_t, Type *);

	public:
		// Create a type declared in Storm.
		Type(TypeFlags flags);

		// Create a type declared in C++.
		Type(TypeFlags flags, Size size, GcType *gcType);

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
