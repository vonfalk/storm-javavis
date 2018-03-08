#pragma once
#include "Compiler/Syntax/Parser.h"
#include "Compiler/Scope.h"

namespace storm {
	namespace bs {
		STORM_PKG(lang.bs);

		class BSLookup : public ScopeLookup {
			STORM_CLASS;
		public:
			// No automatic syntax.
			STORM_CTOR BSLookup();

			// Included packages.
			Array<Package *> *includes;

			// Find things.
			virtual MAYBE(Named *) STORM_FN find(Scope in, SimpleName *name);

			// Add syntax to a parser.
			void STORM_FN addSyntax(Scope from, syntax::ParserBase *to);

		private:
			// Find helper.
			Named *findHelper(Scope from, SimpleName *name);
		};

		// Add includes.
		Bool STORM_FN addInclude(Scope to, Package *p);

		// Add syntax to a parser.
		void STORM_FN addSyntax(Scope from, syntax::ParserBase *to);


	}
}
