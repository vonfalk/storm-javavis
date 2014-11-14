#pragma once
#include "Std.h"

namespace storm {
	namespace bs {
		STORM_PKG(lang.bs);

		/**
		 * Package name from the parser.
		 */
		class Pkg : public Object {
			STORM_CLASS;
		public:
			STORM_CTOR Pkg();

			void STORM_FN add(Auto<Str> part);

			// Get the entire pkg path as a Name.
			Name name() const;

			// All parts so far.
			vector<String> parts;
		};

	}
}
