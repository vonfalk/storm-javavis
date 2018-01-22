#pragma once
#include "Compiler/Template.h"
#include "Compiler/Function.h"

namespace storm {
	STORM_PKG(core.lang);

	/**
	 * Provide a system-wide toS function for value types only implementing operator <<.
	 */
	class ToSTemplate : public Template {
		STORM_CLASS;
	public:
		// Create.
		STORM_CTOR ToSTemplate();

		virtual MAYBE(Named *) STORM_FN generate(SimplePart *part);
	};


	/**
	 * Function generated.
	 */
	class ToSFunction : public Function {
		STORM_CLASS;
	public:
		// Create. Pass the 'operator <<' to use.
		STORM_CTOR ToSFunction(Function *fn);

	private:
		// Output function to use.
		Function *use;

		// Generate code.
		CodeGen *CODECALL create();
	};

}
