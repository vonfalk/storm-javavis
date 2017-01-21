#pragma once
#include "ValueArray.h"
#include "Type.h"

namespace storm {
	STORM_PKG(core.lang);

	// Create the Fn type.
	Type *createFn(Str *name, ValueArray *params);

	/**
	 * Type for function pointers.
	 */
	class FnType : public Type {
		STORM_CLASS;
	public:
		// Create.
		STORM_CTOR FnType(Str *name, ValueArray *params);

	protected:
		// Load.
		virtual Bool STORM_FN loadAll();

	private:
		// Generate the code for calling the function.
		CodeGen *CODECALL callCode();
	};

	// Find the function type.
	Type *fnType(Array<Value> *params);


	// Low-level functionality required by generated machine code.
	void CODECALL fnCallRaw(FnBase *b, void *output, BasicTypeInfo *type, os::FnParams *params, TObject *first);

}
