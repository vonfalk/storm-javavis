#pragma once
#include "Std.h"
#include "NamedThread.h"
#include "Basic/BSFunction.h"
#include "Basic/BSTemplate.h"
#include "Shared/Map.h"

namespace storm {
	namespace bs {
		STORM_PKG(lang.bs);

		/**
		 * Contents of a source file. Contains type and function definitions
		 * for a single source file.
		 */
		class Contents : public ObjectOn<Compiler> {
			STORM_CLASS;
		public:
			STORM_CTOR Contents();

			// Add a type.
			void STORM_FN add(Par<Type> type);

			// Add a function.
			void STORM_FN add(Par<FunctionDecl> fn);

			// Add a named thread.
			void STORM_FN add(Par<NamedThread> thread);

			// Add a template of some kind.
			void STORM_FN add(Par<Template> templ);

			// All types.
			STORM_VAR Auto<ArrayP<Type>> types;

			// All function definitions.
			STORM_VAR Auto<ArrayP<FunctionDecl>> functions;

			// All named threads.
			STORM_VAR Auto<ArrayP<NamedThread>> threads;

			// All templates. TODO: Add STORM_VAR
			Auto<MAP_PP(Str, TemplateAdapter)> templates;
		};

	}
}
