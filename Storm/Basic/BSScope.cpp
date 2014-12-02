#include "stdafx.h"
#include "BSScope.h"
#include "BSBlock.h"
#include "BSVar.h"
#include "PkgReader.h"

namespace storm {

	bs::BSScope::BSScope(Auto<NameLookup> l) : Scope(l), topBlock(null) {}

	Named *bs::BSScope::findHere(const Name &name) const {
		if (name.size() == 1) {
			for (Block *b = topBlock; b; b = b->parent) {
				if (Named *n = b->variable(name[0]))
					return n;
			}
		}

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

	bs::BSScope *bs::BSScope::child(Auto<NameLookup> l) {
		Auto<BSScope> r = CREATE(BSScope, this, l);
		r->file = file;
		r->includes = includes;
		r->topBlock = topBlock;
		return r.ret();
	}

	bs::BSScope *bs::BSScope::child(Auto<Block> b) {
		Auto<BSScope> r = CREATE(BSScope, this, capture(top));
		r->file = file;
		r->includes = includes;
		r->topBlock = b.borrow();
		return r.ret();
	}

}
