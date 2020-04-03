#pragma once
#include "Core/Object.h"

namespace storm {
	namespace syntax {
		STORM_PKG(lang.bnf);

		class Rule;

		/**
		 * Delimiter types in the system in order to create generic implementations of many classes.
		 */
		namespace delim {
			enum Delimiter {
				all,
				optional,
				required
			};
		}


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
