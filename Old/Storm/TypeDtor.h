#pragma once
#include "Function.h"
#include "Code.h"

namespace storm {
	STORM_PKG(core.lang);

	/**
	 * A default destructor for a type. This is provided since it is useful in
	 * many cases, especially when debugging. Real language implementations
	 * probably want to have their own implementations.
	 *
	 * The default constructor does not take any parameters, and assumes that
	 * any base classes has a default constructor as well. The constructor
	 * throws an exception otherwise.
	 */
	class TypeDefaultDtor : public Function {
		STORM_CLASS;
	public:
		// Create the constructor.
		TypeDefaultDtor(Type *owner);

	private:
		// Generate code.
		void generateCode(Type *type, Function *before);
	};


	/**
	 * Return a pointer to a function that can redirect the destructor to
	 * a regular void (CODECALL *)(void *) stored in the 0:th slot of the
	 * Storm vtable.
	 */
	void *dtorRedirect();


	/**
	 * Wrap a raw destructor from C++ (not through any other wrappers). Highly
	 * system specific, do not save the code from here!
	 */
	Code *wrapRawDestructor(Engine &e, void *ptr);

}
