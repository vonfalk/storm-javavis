#include "stdafx.h"
#include "Scope.h"
#include "Named.h"
#include "Overload.h"
#include "Package.h"

namespace storm {

	Package *Scope::firstPkg(NameLookup *l) {
		while (!as<Package>(l))
			l = l->parent();
		return as<Package>(l);
	}

	Package *Scope::rootPkg(Package *p) {
		while (p->parent())
			p = p->parent();
		return p;
	}

	Package *Scope::corePkg(NameLookup *l) {
		Package *top = firstPkg(l);
		Package *root = rootPkg(top);
		return as<Package>(root->find(Name(L"core")));
	}

	NameLookup *Scope::nextCandidate(NameLookup *prev) {
		if (Package *pkg = as<Package>(prev)) {
			// Examine the root next.
			Package *root = rootPkg(pkg);
			if (pkg == root)
				return null;
			else
				return root;
		} else {
			// Default behaviour is to look at the nearest parent.
			return prev->parent();
		}
	}

	Scope::Scope(Auto<NameLookup> top) : top(top.borrow()) {}

	Named *Scope::find(const Name &name) const {
		return findHere(name);
	}

	NameOverload *Scope::find(const Name &name, const vector<Value> &params) const {
		return findHere(name, params);
	}

	Named *Scope::findHere(const Name &name) const {
		// Regular path.
		for (NameLookup *at = top; at; at = nextCandidate(at)) {
			if (Named *found = at->find(name))
				return found;
		}

		// Core.
		if (Package *core = corePkg(top))
			if (Named *found = core->find(name))
				return found;

		// We failed!
		return null;
	}

	NameOverload *Scope::findHere(const Name &name, const vector<Value> &params) const {
		Named *named = find(name);

		if (Overload *overload = as<Overload>(named)) {
			return overload->find(params);
		} else {
			return null;
		}
	}

	/**
	 * Extra
	 */

	ScopeExtra::ScopeExtra(Auto<NameLookup> lookup) : Scope(lookup) {}

	Named *ScopeExtra::findHere(const Name &name) const {
		if (Named *found = Scope::findHere(name))
			return found;

		for (nat i = 0; i < extra.size(); i++) {
			if (Named *found = extra[i]->find(name))
				return found;
		}

		return null;
	}

}
