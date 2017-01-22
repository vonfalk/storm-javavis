#pragma once
#include "Std.h"
#include "Scope.h"
#include "Shared/Io/Url.h"
#include "Package.h"
#include "Syntax/Parser.h"

namespace storm {
	namespace bs {
		STORM_PKG(lang.bs);

		class Block;

		class BSScope : public ScopeLookup {
			STORM_CLASS;
		public:
			// No atuomatic syntax.
			STORM_CTOR BSScope();

			// Included packages. (risk of cycles here)
			vector<Package *> includes;

			// Find stuff.
			virtual Named *STORM_FN find(const Scope &from, Par<SimpleName> name);

			// Add syntax to a parser.
			void addSyntax(const Scope &from, Par<syntax::ParserBase> to);

		private:
			// Find helper.
			virtual Named *findHelper(const Scope &from, Par<SimpleName> name);
		};

		Bool STORM_FN addInclude(const Scope &to, Par<Package> p);

		// Add syntax to a Parser.
		void STORM_FN addSyntax(Scope from, Par<syntax::ParserBase> to);

	}
}
