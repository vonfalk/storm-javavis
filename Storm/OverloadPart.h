#pragma once
#include "Name.h"

namespace storm {

	class Function;

	/**
	 * Type part that is used to match overloads of a specific function. It simply ignores the first
	 * parameter, as the type of the first parameter is a more specific class, and would not
	 * otherwise match.
	 */
	class OverloadPart : public SimplePart {
		STORM_CLASS;
	public:
		// Match overloads to this function.
		STORM_CTOR OverloadPart(Par<Function> fn);

		// See if 'named' matches what we were interested in.
		virtual Int STORM_FN matches(Par<Named> candidate);
	};

}
