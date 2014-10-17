#include "stdafx.h"
#include "Scope.h"

namespace storm {

	Scope::Scope() : nameFallback(null) {}

	Named *Scope::find(const Name &name) {
		Named *r = findHere(name);
		if (r == null && nameFallback && nameFallback != this)
			return nameFallback->find(name);
		else
			return r;
	}


	Named *ScopeChain::findHere(const Name &name) {
		for (nat i = 0; i < scopes.size(); i++) {
			Named *n = scopes[i]->find(name);
			if (n)
				return n;
		}
		return null;
	}
}
