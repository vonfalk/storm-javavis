#pragma once
#include "Type.h"
#include "Parse.h"

namespace storm {
	namespace syntax {

		/**
		 * Syntax option.
		 *
		 * This is represented as a type, which inherits from a Rule type. This class will override
		 * the 'eval' member of the parent type.
		 */
		class Option : public Type {
			STORM_CLASS;
		public:
			// Create.
			STORM_CTOR Option(Par<Str> name, Par<OptionDecl> decl, Scope scope);

			// Declared at.
			STORM_VAR SrcPos pos;

			// Scope.
			STORM_VAR Scope scope;

			// More to come...

			// Load contents.
			virtual Bool STORM_FN loadAll();
		};

	}
}
