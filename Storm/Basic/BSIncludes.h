#pragma once
#include "Std.h"
#include "BSPkg.h"

namespace storm {
	namespace bs {
		STORM_PKG(lang.bs);

		/**
		 * Receives includes from the parser.
		 */
		class Includes : public Object {
			STORM_CLASS;
		public:
			STORM_CTOR Includes();

			void STORM_FN add(Auto<Pkg> pkg);

			// Names.
			vector<Name> names;
		};

	}
}
