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
		void generateCode(Type *type, Function *before);
	};


	/**
	 * A default copy-constructor for a type. This is provided since it is useful
	 * in many cases, especially when debugging. Real language implementations probably
	 * want to have their own implementations.
	 *
	 * The copy constructor works just like in C++. It takes one (reference) parameter
	 * that is the source of the copy.
	 * TODO: Make use of "copy"-like semantics when dealing with Objects.
	 * NOTE: Assumes that all members have been added by the time this class is created!
	 */
	class TypeCopyCtor : public Function {
		STORM_CLASS;
	public:
		// Create.
		TypeCopyCtor(Type *owner);

	private:
		// Generate code.
		void generateCode(Type *type, Function *before);
	};


	/**
	 * A default assignment-operator for a value. This is provided since it is useful
	 * in many cases, especially when debugging. Real language implementations probably
	 * want to have their own implementations.
	 *
	 * The copy constructor works just like in C++. It takes one (reference) parameter
	 * that is the source of the copy.
	 * NOTE: Assumes that all members have been added by the time this class is created!
	 */
	class TypeAssignFn : public Function {
		STORM_CLASS;
	public:
		// Create.
		TypeAssignFn(Type *owner);

	private:
		// Generate code.
		void generateCode(Type *type, Function *before);
	};


	/**
	 * Generate a default clone function for the type passed. Used with a template.
	 */
	Named *stdClone(Par<NamePart> par);

	// Create the template.
	Template *cloneTemplate(Engine &to);

}
