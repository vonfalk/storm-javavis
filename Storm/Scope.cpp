#include "stdafx.h"
#include "Scope.h"
#include "Named.h"
#include "Overload.h"
#include "Package.h"

namespace storm {

	static Package *rootPkg(Package *p) {
		while (p->parent())
			p = p->parent();
		return p;
	}

	static Package *corePkg(NameLookup *l) {
		while (!as<Package>(l))
			l = l->parent();
		Package *root = rootPkg(as<Package>(l));
		return as<Package>(root->find(Name(L"core")));
	}

	static NameLookup *nextCandidate(NameLookup *prev) {
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

	Scope::Scope(NameLookup *top) : top(top) {
		if (top)
			if (Package *core = corePkg(top))
				extra.push_back(core);
	}

	Named *Scope::find(const Name &name) const {
		// Regular path.
		for (NameLookup *at = top; at; at = nextCandidate(at)) {
			if (Named *found = at->find(name))
				return found;
		}

		for (nat i = 0; i < extra.size(); i++) {
			if (Named *found = extra[i]->find(name))
				return found;
		}

		// We failed!
		return null;
	}

	NameOverload *Scope::find(const Name &name, const vector<Value> &params) const {
		Named *named = find(name);

		if (Overload *overload = as<Overload>(named)) {
			return overload->find(params);
		} else {
			return null;
		}
	}

}
