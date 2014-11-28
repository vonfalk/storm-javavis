#pragma once
#include "Std.h"
#include "SyntaxSet.h"

namespace storm {
	namespace bs {

		class BSScope : public Scope {
			STORM_CLASS;
		public:
			STORM_CTOR BSScope(Auto<NameLookup> l);

			virtual Named *find(const Name &name) const;

			// File name.
			Path file;

			// Included packages. (risk of cycles here)
			vector<Package *> includes;

			// Get syntax.
			void addSyntax(SyntaxSet &to);
		};

	}
}
