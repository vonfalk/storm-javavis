#pragma once
#include "Std.h"
#include "SyntaxSet.h"

namespace storm {
	namespace bs {
		class Block;

		class BSScope : public Scope {
			STORM_CLASS;
		public:
			STORM_CTOR BSScope(Auto<NameLookup> l);

			// File name.
			Path file;

			// Included packages. (risk of cycles here)
			vector<Package *> includes;

			// Current block (cycle risk).
			Block *topBlock;

			// Get syntax.
			void addSyntax(SyntaxSet &to);

			// Sub-scope. (TODO: Redo to make operations like this easier to implement, and cheaper).
			BSScope *child(Auto<NameLookup> l);
			BSScope *child(Auto<Block> block);

		protected:
			virtual Named *findHere(const Name &name) const;
			virtual Named *findHere(const Name &name, const vector<Value> &param) const;
		};

	}
}
