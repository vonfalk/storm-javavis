#include "stdafx.h"
#include "BSScope.h"
#include "BSBlock.h"
#include "BSVar.h"
#include "PkgReader.h"

namespace storm {

	bs::BSScope::BSScope(const Path &file) : file(file) {}

	Named *bs::BSScope::find(const Scope &s, const Name &name) {
		if (Named *found = ScopeLookup::find(s, name))
			return found;

		for (nat i = 0; i < includes.size(); i++) {
			if (Named *found = includes[i]->find(name))
				return found;
		}

		return null;
	}

	Named *bs::BSScope::find(const Scope &s, const Name &name, const vector<Value> &params) {
		if (Named *n = ScopeLookup::find(s, name, params))
			return n;

		// Expressions of the form foo(x, y, z) are equal to x.foo(y, z).
		if (params.size() > 0 && name.size() == 1) {
			if (params[0] != Value()) {
				Named *n = params[0].type->find(name);
				if (Overload *o = as<Overload>(n)) {
					return o->find(params);
				} else if (params.size() == 0) {
					return n;
				}
			}
		}

		return null;
	}

	void bs::BSScope::addSyntax(const Scope &from, SyntaxSet &to) {
		to.add(*engine().package(syntaxPkg(file)));
		to.add(*firstPkg(from.top)); // current package.

		for (nat i = 0; i < includes.size(); i++)
			to.add(*includes[i]);
	}

	void bs::addInclude(const Scope &to, Package *pkg) {
		if (Auto<BSScope> s = to.lookup.as<BSScope>()) {
			s->includes.push_back(pkg);
		} else {
			WARNING(L"This is not what you want to do!");
		}
	}

	void bs::addSyntax(const Scope &scope, SyntaxSet &to) {
		if (Auto<BSScope> s = scope.lookup.as<BSScope>()) {
			s->addSyntax(scope, to);
		} else {
			WARNING(L"This is probably not what you want to do!");
		}
	}

}
