#pragma once
#include "Array.h"
#include "Template.h"
#include "Type.h"
#include "Storm/CodeGen.h"

namespace storm {
	STORM_PKG(core);

	// Add the array template class to the package given.
	void addArrayTemplate(Par<Package> to);

	/**
	 * The array type.
	 */
	class ArrayType : public Type {
		STORM_CLASS;
	public:
		// Ctor.
		ArrayType(const Value &param);

		// Parameter type.
		const Value param;

	protected:
		// Lazy loading.
		virtual void lazyLoad();

	private:

		// Load functions assuming param is an object.
		void loadClassFns();

		// Load functions assuming param is a value.
		void loadValueFns();

		// Generate the toS function for values.
		CodeGen *CODECALL valueToS();

		// Generate the toS function for values, assuming that we have to call
		// the add() function on the StrBuf object.
		CodeGen *CODECALL valueToSAdd(Function *addFn);

		// Generate the toS function for values, assuming that we should use the toS
		// function of the object itself.
		CodeGen *CODECALL valueToSMember(Function *member);
	};

	// See Array.h for 'arrayType' function.

}
