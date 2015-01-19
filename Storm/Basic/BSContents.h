#pragma once
#include "Std.h"
#include "Basic/BSFunction.h"

namespace storm {
	namespace bs {
		STORM_PKG(lang.bs);

		/**
		 * Contents of a source file. Contains type and function definitions
		 * for a single source file.
		 */
		class Contents : public Object {
			STORM_CLASS;
		public:
			STORM_CTOR Contents();

			// Add a type.
			void STORM_FN add(Par<Type> type);

			// Add a function.
			void STORM_FN add(Par<FunctionDecl> fn);

			// All types.
			vector<Auto<Type> > types;

			// All function definitions.
			vector<Auto<FunctionDecl> > functions;

			// Set the scope for all contents that needs it.
			void setScope(const Scope &scope);
		};

	}
}
