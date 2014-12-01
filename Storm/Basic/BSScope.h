#pragma once
#include "Std.h"
#include "SyntaxSet.h"

namespace storm {
	namespace bs {

		class BSScope : public Scope {
			STORM_CLASS;
		public:
			STORM_CTOR BSScope(Auto<NameLookup> l);

			// File name.
			Path file;

			// Included packages. (risk of cycles here)
			vector<Package *> includes;

			// Get syntax.
			void addSyntax(SyntaxSet &to);

		protected:
			virtual Named *findHere(const Name &name) const;
			virtual NameOverload *findHere(const Name &name, const vector<Value> &param) const;
		};

	}
}
