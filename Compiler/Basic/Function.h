#pragma once
#include "Compiler/NamedThread.h"

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
			STORM_CTOR FunctionDecl();
		};

	}
}
