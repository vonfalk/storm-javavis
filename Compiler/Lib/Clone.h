#pragma once
#include "Template.h"
#include "Core/CloneEnv.h"

namespace storm {
	STORM_PKG(core.lang);

	class Function;
	class Type;

	/**
	 * Template used to generate clone() for all types in the system.
	 */
	class CloneTemplate : public Template {
		STORM_CLASS;
	public:
		// Create.
		STORM_CTOR CloneTemplate();

		virtual MAYBE(Named *) STORM_FN generate(SimplePart *part);
	};

	// Find 'core.clone' for a type.
	Function *cloneFn(Type *t);

	// Find 'core.clone' (taking a CloneEnv) for a type.
	Function *cloneFnEnv(Type *t);

}
