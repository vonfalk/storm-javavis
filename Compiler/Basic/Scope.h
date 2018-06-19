#pragma once
#include "Compiler/Named.h"

namespace storm {
	namespace bs {
		STORM_PKG(lang.bs);

		/**
		 * Dummy scope used to indicate which file a certain scope belongs to.
		 *
		 * Used when resolving types at top-level (such as parameters to functions) so that they are
		 * able to access file private variables correctly.
		 *
		 * Note: This can likely be removed if we decide to have a SrcPos inside Named.
		 */
		class FileScope : public NameLookup {
			STORM_CLASS;
		public:
			// Create.
			STORM_CTOR FileScope(NameLookup *parent, SrcPos pos);

			// Position.
			SrcPos pos;
		};

		// Create a scope that wraps a FileScope.
		Scope STORM_FN fileScope(Scope scope, SrcPos pos);

	}
}
