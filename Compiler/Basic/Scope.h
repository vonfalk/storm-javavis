#pragma once
#include "Compiler/Named.h"

namespace storm {
	namespace bs {
		STORM_PKG(lang.bs);

		// Create a scope that can access things which are file private.
		inline Scope STORM_FN fileScope(Scope scope, SrcPos pos) {
			return scope.withPos(pos);
		}

	}
}
