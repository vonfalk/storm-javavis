#pragma once
#include "Compiler/Lib/Enum.h"
#include "Compiler/Syntax/SStr.h"

namespace storm {
	namespace bs {
		STORM_PKG(lang.bs);

		// Create an enum type from the syntax.
		Enum *STORM_FN createEnum(syntax::SStr *name, MAYBE(Str *) bitmask) ON(Compiler);

		/**
		 * Context used when creating an enum to keep track of the next label etc.
		 */
		class EnumContext : public ObjectOn<Compiler> {
			STORM_CLASS;
		public:
			// Create. Use "context" below.
			EnumContext(Enum *e);

			// Create an enum value.
			EnumValue *STORM_FN create(syntax::SStr *name);
			EnumValue *STORM_FN create(syntax::SStr *name, Nat value);

		private:
			// Enum we're working with.
			Enum *e;

			// Counter for the next value in the sequence.
			Nat next;

			// Overflow?
			Bool overflow;
		};

		// Create a context.
		EnumContext *STORM_FN context(Enum *e) ON(Compiler);

	}
}
