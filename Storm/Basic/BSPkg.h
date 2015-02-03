#pragma once
#include "Std.h"
#include "SyntaxObject.h"

namespace storm {
	namespace bs {
		STORM_PKG(lang.bs);

		/**
		 * Package name from the parser.
		 * TODO? Replace with a plain Name?
		 */
		class Pkg : public SObject {
			STORM_CLASS;
		public:
			STORM_CTOR Pkg();

			void STORM_FN add(Par<SStr> part);

			// Get the entire pkg path as a Name.
			Name *STORM_FN name() const;

		private:
			// Name under construction.
			Auto<Name> n;
		};

	}
}
