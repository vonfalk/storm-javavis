#pragma once
#include "Compiler/NamedThread.h"
#include "Function.h"

namespace storm {
	namespace bs {
		STORM_PKG(lang.bs);

		/**
		 * Content of a single source file.
		 */
		class Content : public ObjectOn<Compiler> {
			STORM_CLASS;
		public:
			STORM_CTOR Content();

			// Add a type.
			void STORM_FN add(Type *type);

			// Add a function.
			void STORM_FN add(FunctionDecl *fn);

			// Add a named thread.
			void STORM_FN add(NamedThread *thread);

			// Add a template of some kind.
			// void STORM_FN add(Template *templ);

			// Add either of the above types.
			void STORM_FN add(TObject *obj);

			// All types.
			Array<Type *> *types;

			// All function definitions.
			Array<FunctionDecl *> *functions;

			// All named threads.
			Array<NamedThread *>*threads;

			// All templates.
			// Map<Str *, TemplateAdapter *> *templates;
		};

	}
}