#pragma once
#include "Std.h"
#include "SyntaxObject.h"
#include "Name.h"

namespace storm {
	namespace bs {
		STORM_PKG(lang.bs);

		class TypeName;

		/**
		 * Parameters to a type.
		 */
		class TypePart : public SObject {
			STORM_CLASS;
		public:
			// Ctor.
			STORM_CTOR TypePart(Par<SStr> name);

			// SStr.
			STORM_CTOR TypePart(Par<Str> name);

			// Add parameter.
			void STORM_FN add(Par<TypeName> t);

			// Name.
			Auto<Str> name;

			// Parameters.
			vector<Auto<TypeName> > params;

			// Name.
			Str *STORM_FN title() const;

			// Size.
			Nat STORM_FN count() const;

			// Element access.
			TypeName *STORM_FN operator [](Nat id) const;

			// As name part.
			NamePart *toPart(const Scope &scope);

		protected:
			// Output.
			virtual void output(wostream &to) const;
		};

		/**
		 * Type name.
		 */
		class TypeName : public SObject {
			STORM_CLASS;
		public:
			// Ctor.
			STORM_CTOR TypeName();

			// Add a part.
			void STORM_FN add(Par<TypePart> part);

			// Parts.
			vector<Auto<TypePart> > parts;

			// Size.
			Nat STORM_FN count() const;

			// Get element.
			TypePart *STORM_FN operator [](Nat id) const;

			// Convert to a Name.
			Name *toName(const Scope &scope);

			// Find.
			Named *find(const Scope &scope);

			// Resolve to a type.
			Value resolve(const Scope &scope);

		protected:
			// Output.
			virtual void output(wostream &to) const;
		};

	}
}
