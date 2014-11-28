#include "stdafx.h"
#include "BSScope.h"
#include "PkgReader.h"

namespace storm {

	bs::BSScope::BSScope(Auto<NameLookup> l) : Scope(l) {}

	Named *bs::BSScope::find(const Name &name) const {
		if (Named *found = Scope::find(name))
			return found;

		for (nat i = 0; i < includes.size(); i++) {
			if (Named *found = includes[i]->find(name))
				return found;
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
