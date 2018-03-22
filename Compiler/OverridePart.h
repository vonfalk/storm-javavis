#pragma once
#include "NamePart.h"
#include "Value.h"

namespace storm {
	STORM_PKG(core.lang);

	class Named;
	class Function;
	class Type;

	/**
	 * Lookup used for finding functions in superclasses which 'match' overrides. Can be created
	 * once and used in the entire inheritance chain.
	 */
	class OverridePart : public SimplePart {
		STORM_CLASS;
	public:
		// Create, specifying the function used.
		STORM_CTOR OverridePart(Function *match);

		// Create, specifying the function used and a new owning class.
		STORM_CTOR OverridePart(Type *parent, Function *match);

		// Custom badness measure.
		virtual Int STORM_FN matches(Named *candidate, Scope scope) const;

		// Custom to string.
		virtual void STORM_FN toS(StrBuf *to) const;

	private:
		// Remember the result as well.
		Value result;
	};


}
