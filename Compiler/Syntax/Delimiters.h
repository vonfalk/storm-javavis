#pragma once
#include "Core/Object.h"

namespace storm {
	namespace syntax {
		STORM_PKG(lang.bnf);

		class Rule;

		/**
		 * Set of delimiters used in a file.
		 */
		class Delimiters : public Object {
			STORM_CLASS;
		public:
			// Create.
			Delimiters();

			// Optional delimiter (used for ",").
			MAYBE(Rule *) optional;

			// Required delimiter (used for "~").
			MAYBE(Rule *) required;
		};

	}
}
