#pragma once
#include "Function.h"

namespace storm {
	STORM_PKG(core.lang);

	/**
	 * Default constructor for a type.
	 *
	 * The constructor takes no parameters, assumes the base class has a default constructor and
	 * tries to initialize all data members using their default constructor.
	 */
	class TypeDefaultCtor : public Function {
		STORM_CLASS;
	public:
		// Create.
		STORM_CTOR TypeDefaultCtor(Type *owner);
	};


	/**
	 * Default copy-constructor for a type.
	 */
	class TypeCopyCtor : public Function {
		STORM_CLASS;
	public:
		// Create.
		STORM_CTOR TypeCopyCtor(Type *owner);
	};


	/**
	 * Default assignment operator for a value.
	 */
	class TypeAssign : public Function {
		STORM_CLASS;
	public:
		// Create.
		STORM_CTOR TypeAssign(Type *owner);
	};

	/**
	 * Default deepCopy function.
	 */
	class TypeDeepCopy : public Function {
		STORM_CLASS;
	public:
		// Create.
		STORM_CTOR TypeDeepCopy(Type *owner);
	};

}
