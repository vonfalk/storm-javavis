#pragma once
#include "Std.h"
#include "BSType.h"
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

			void STORM_FN add(Par<TypeName> pkg);

			// Names.
			vector<Auto<TypeName> > names;
		};

	}
}
