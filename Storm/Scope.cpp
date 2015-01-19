#include "stdafx.h"
#include "Scope.h"
#include "Named.h"
#include "Overload.h"
#include "Package.h"
#include "Engine.h"
#include "Type.h"

namespace storm {

	/**
	 * Scope.
	 */

	Scope::Scope() : top(null), lookup(null) {}

	Scope::Scope(Par<NameLookup> top) : top(top.borrow()) {
		assert(top);
		lookup = top->engine().scopeLookup();
	}

	Scope::Scope(Par<NameLookup> top, Par<ScopeLookup> lookup) : top(top.borrow()), lookup(lookup) {}

	Scope::Scope(const Scope &parent, Par<NameLookup> top) : top(top.borrow()), lookup(parent.lookup) {}

	Named *Scope::find(const Name &name) const {
		if (lookup)
			return lookup->find(*this, name);

		return null;
	}

	Named *Scope::find(const Name &name, const vector<Value> &params) const {
		if (lookup)
			return lookup->find(*this, name, params);

		return null;
	}


	/**
	 * ScopeLookup.
	 */

	ScopeLookup::ScopeLookup() {}

	Package *ScopeLookup::firstPkg(NameLookup *l) {
		while (!as<Package>(l))
			l = l->parent();
		return as<Package>(l);
	}

	Package *ScopeLookup::rootPkg(Package *p) {
		while (p->parent())
			p = p->parent();
		return p;
	}

	Package *ScopeLookup::corePkg(NameLookup *l) {
		Package *top = firstPkg(l);
		Package *root = rootPkg(top);
		return as<Package>(root->find(Name(L"core")));
	}

	NameLookup *ScopeLookup::nextCandidate(NameLookup *prev) {
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

	Named *ScopeLookup::find(const Scope &in, const Name &name) {
		// Regular path.
		for (NameLookup *at = in.top; at; at = nextCandidate(at)) {
			if (Named *found = at->find(name))
				return found;
		}

		// Core.
		if (Package *core = corePkg(in.top))
			if (Named *found = core->find(name))
				return found;

		// We failed!
		return null;
	}

	Named *ScopeLookup::find(const Scope &in, const Name &name, const vector<Value> &params) {
		Named *named = find(in, name);

		if (Overload *overload = as<Overload>(named)) {
			return overload->find(params);
		} else if (params.size() == 0) {
			return named;
		} else {
			return null;
		}
	}

	/**
	 * Extra
	 */

	ScopeExtra::ScopeExtra() {}

	Named *ScopeExtra::find(const Scope &in, const Name &name) {
		if (Named *found = ScopeLookup::find(in, name))
			return found;

		for (nat i = 0; i < extra.size(); i++) {
			if (Named *found = extra[i]->find(name))
				return found;
		}

		return null;
	}

}
