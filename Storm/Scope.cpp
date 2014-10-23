#include "stdafx.h"
#include "Scope.h"
#include "Named.h"
#include "Overload.h"

namespace storm {

	Scope::Scope() : nameFallback(null) {}

	Named *Scope::find(const Name &name) {
		Named *r = findHere(name);
		if (r == null && nameFallback && nameFallback != this)
			return nameFallback->find(name);
		else
			return r;
	}

	NameOverload *Scope::find(const Name &name, const vector<Value> &params) {
		Named *named = find(name);

		if (Overload *overload = as<Overload>(named)) {
			return overload->find(params);
		} else {
			return null;
		}
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
