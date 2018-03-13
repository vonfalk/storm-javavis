#pragma once
#include "Compiler/Basic/Doc.h"

namespace storm {
	namespace syntax {
		STORM_PKG(lang.bnf);

		/**
		 * Documentation for the syntax language. Based of the Basic Storm documentation.
		 */
		class SyntaxDoc : public bs::BSDoc {
			STORM_CLASS;
		public:
			STORM_CTOR SyntaxDoc(SrcPos docPos, Named *entity);

			// Get documentation.
			virtual Doc *STORM_FN get();
		};

	}
}
