#pragma once
#include "Storm/Std.h"
#include "Storm/SyntaxObject.h"
#include "Storm/Name.h"
#include "Storm/Named.h"

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

			// Create from a NamePart.
			STORM_CTOR TypePart(Par<NamePart> name);

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

			// Convert from a Value (maybe remove?)
			STORM_CTOR TypeName(Value from);

			// Add a part.
			void STORM_FN add(Par<TypePart> part);

			// Parts.
			vector<Auto<TypePart> > parts;

			// Size.
			Nat STORM_FN count() const;

			// Get element.
			TypePart *STORM_FN operator [](Nat id) const;

			// Convert to a Name.
			Name *STORM_FN toName(const Scope &scope);

			// Find.
			MAYBE(Named) *STORM_FN find(const Scope &scope);

			// Resolve to a type.
			Value STORM_FN resolve(const Scope &scope);

		protected:
			// Output.
			virtual void output(wostream &to) const;
		};

	}
}
