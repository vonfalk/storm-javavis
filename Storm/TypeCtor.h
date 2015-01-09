#pragma once
#include "Function.h"

namespace storm {
	STORM_PKG(core.lang);

	/**
	 * A default constructor for a type. This is provided since it is useful in
	 * many cases, especially when debugging. Real language implementations
	 * probably want to have their own implementations.
	 *
	 * The default constructor does not take any parameters, and assumes that
	 * any base classes has a default constructor as well. The constructor
	 * throws an exception otherwise.
	 */
	class TypeDefaultCtor : public Function {
		STORM_CLASS;
	public:
		// Create the constructor.
		TypeDefaultCtor(Type *owner);

	private:
		// Generate code.
		void generateCode(Function *before);
	};

}
