#include "stdafx.h"
#include "Scope.h"

namespace storm {

	Scope::Scope() : nameFallback(null) {}

	Named *Scope::find(const Name &name) {
		Named *r = findHere(name);
		if (r == null && nameFallback)
			return nameFallback->find(name);
		else
			return r;
	}


}
