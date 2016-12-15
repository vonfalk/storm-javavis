#pragma once
#include "Compiler/NamedThread.h"
#include "Compiler/Function.h"
#include "Compiler/Syntax/SStr.h"
#include "Compiler/Syntax/Node.h"
#include "Param.h"

namespace storm {
	namespace bs {
		STORM_PKG(lang.bs);

		/**
		 * Function declaration. This holds the needed information to create each function later
		 * on. Also acts as an intermediate store for types until we have found all types in the
		 * current package. Otherwise, we would behave like C, that the type declarations have to be
		 * before anything that uses them.
		 */
		class FunctionDecl : public ObjectOn<Compiler> {
			STORM_CLASS;
		public:
			STORM_CTOR FunctionDecl(Scope scope,
									SrcName *result,
									syntax::SStr *name,
									Array<NameParam> *params,
									syntax::Node *body);

			STORM_CTOR FunctionDecl(Scope scope,
									SrcName *result,
									syntax::SStr *name,
									Array<NameParam> *params,
									SrcName *thread,
									syntax::Node *body);

			// Values.
			Scope scope;
			syntax::SStr *name;
			SrcName *result;
			Array<NameParam> *params;
			MAYBE(SrcName *) thread;
			syntax::Node *body;

			// Create the corresponding function.
			Function *STORM_FN createFn();
		};


	}
}
