#pragma once
#include "Std.h"
#include "SyntaxObject.h"
#include "Name.h"

namespace storm {
	namespace bs {
		STORM_PKG(lang.bs);

		/**
		 * Names a package.
		 */
		class PkgName : public SObject {
			STORM_CLASS;
		public:
			STORM_CTOR PkgName();

			void STORM_FN add(Par<SStr> part);

			vector<Auto<Str> > parts;

		protected:
			virtual void output(wostream &to) const;
		};

		/**
		 * Names a type.
		 */
		class TypeName : public SObject {
			STORM_CLASS;
		public:
			STORM_CTOR TypeName(Par<SStr> name);
			STORM_CTOR TypeName(Par<PkgName> pkg, Par<SStr> name);

			Auto<PkgName> pkg;
			Auto<Str> name;

			// Get a regular name.
			Name getName();

			// Find the Value.
			Value value(const Scope &scope);

		protected:
			virtual void output(wostream &to) const;
		};

	}
}
