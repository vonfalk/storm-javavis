#pragma once
#include "Std.h"
#include "BSPkg.h"
#include "SyntaxObject.h"

namespace storm {
	namespace bs {
		STORM_PKG(lang.bs);

		/**
		 * Receives includes from the parser.
		 */
		class Includes : public SObject {
			STORM_CLASS;
		public:
			STORM_CTOR Includes();

			void STORM_FN add(Par<Pkg> pkg);

			// Names.
			vector<Name> names;
		};

	}
}
