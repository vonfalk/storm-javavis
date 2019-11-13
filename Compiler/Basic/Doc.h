#pragma once
#include "Core/SrcPos.h"
#include "Compiler/Doc.h"

namespace storm {
	namespace bs {
		STORM_PKG(lang.bs);

		/**
		 * Documentation for Storm functions.
		 */
		class BSDoc : public NamedDoc {
			STORM_CLASS;
		public:
			STORM_CTOR BSDoc(SrcPos docPos, Named *entity);

			// Get documentation.
			virtual Doc *STORM_FN get();

		protected:
			// Where is the documentation located?
			SrcPos docPos;

			// Which object?
			Named *entity;

		private:
			// Read the body of the documentation.
			Str *readBody();

			// Create parameters from 'entity' if possible.
			Array<DocParam> *createParams();
		};


		/**
		 * Apply documentation to supported objects.
		 */
		TObject *STORM_FN applyDoc(SrcPos start, TObject *to);

	}
}
