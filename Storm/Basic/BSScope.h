#pragma once
#include "Std.h"
#include "SyntaxSet.h"

namespace storm {
	namespace bs {
		STORM_PKG(lang.bs);

		class Block;

		class BSScope : public ScopeLookup {
			STORM_CLASS;
		public:
			BSScope(Par<Url> file);

			// File name.
			Auto<Url> file;

			// Included packages. (risk of cycles here)
			vector<Package *> includes;

			// Get syntax.
			void addSyntax(const Scope &from, Par<SyntaxSet> to);

			// Find stuff.
			virtual Named *find(const Scope &from, Par<Name> name);

		private:
			// Find helper.
			virtual Named *findHelper(const Scope &from, Par<Name> name);
		};

		void addInclude(const Scope &to, Package *p);

		// Get syntax from a scope.
		SyntaxSet *STORM_FN getSyntax(const Scope &from);

	}
}
