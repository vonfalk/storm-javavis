#include "stdafx.h"
#include "Scope.h"

namespace storm {
	namespace bs {

		FileScope::FileScope(NameLookup *parent, SrcPos pos) : NameLookup(parent), pos(pos) {}

		Scope fileScope(Scope scope, SrcPos pos) {
			if (!scope.top)
				return scope;

			return scope.child(new (scope.top) FileScope(scope.top, pos));
		}

	}
}
