#pragma once
#include "NamePart.h"

namespace storm {
	STORM_PKG(core.lang);

	/**
	 * Lookup enforcing exact matches. Also ignores the scope.
	 */
	class ExactPart : public SimplePart {
		STORM_CLASS;
	public:
		STORM_CTOR ExactPart(Str *name, Array<Value> *parts);

		// Custom badness measure.
		virtual Int STORM_FN matches(Named *candidate, Scope scope) const;
	};

}
