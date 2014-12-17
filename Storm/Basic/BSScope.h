#pragma once
#include "Std.h"
#include "SyntaxSet.h"

namespace storm {
	namespace bs {
		class Block;

		class BSScope : public ScopeLookup {
			STORM_CLASS;
		public:
			BSScope(const Path &file);

			// File name.
			const Path file;

			// Included packages. (risk of cycles here)
			vector<Package *> includes;

			// Get syntax.
			void addSyntax(const Scope &from, SyntaxSet &to);

			// Find stuff.
			virtual Named *find(const Scope &from, const Name &name);
			virtual Named *find(const Scope &from, const Name &name, const vector<Value> &param);
		};

		void addInclude(const Scope &to, Package *p);
		void addSyntax(const Scope &from, SyntaxSet &to);

	}
}
