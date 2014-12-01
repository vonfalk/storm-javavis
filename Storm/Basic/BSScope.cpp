#include "stdafx.h"
#include "BSScope.h"
#include "PkgReader.h"

namespace storm {

	bs::BSScope::BSScope(Auto<NameLookup> l) : Scope(l) {}

	Named *bs::BSScope::findHere(const Name &name) const {
		if (Named *found = Scope::findHere(name))
			return found;

		for (nat i = 0; i < includes.size(); i++) {
			if (Named *found = includes[i]->find(name))
				return found;
		}

		return null;
	}

	NameOverload *bs::BSScope::findHere(const Name &name, const vector<Value> &params) const {
		if (NameOverload *n = Scope::findHere(name, params))
			return n;

		if (params.size() > 0 && name.size() == 1) {
			if (Overload *o = as<Overload>(params[0].type->find(name)))
				return o->find(params);
		}

		return null;
	}

	void bs::BSScope::addSyntax(SyntaxSet &to) {
		to.add(*engine().package(syntaxPkg(file)));
		to.add(*firstPkg(top)); // current package.

		for (nat i = 0; i < includes.size(); i++)
			to.add(*includes[i]);
	}

}
