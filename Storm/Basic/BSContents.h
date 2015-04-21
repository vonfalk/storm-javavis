#pragma once
#include "Std.h"
#include "NamedThread.h"
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

			// Add a named thread.
			void STORM_FN add(Par<NamedThread> thread);

			// All types.
			STORM_VAR Auto<ArrayP<Type>> types;

			// All function definitions.
			STORM_VAR Auto<ArrayP<FunctionDecl>> functions;

			// All named threads.
			STORM_VAR Auto<ArrayP<NamedThread>> threads;

			// Set the scope for all contents that needs it.
			void setScope(const Scope &scope);
		};

	}
}
