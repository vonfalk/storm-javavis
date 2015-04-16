#include "stdafx.h"
#include "Scope.h"
#include "Named.h"
#include "Package.h"
#include "Engine.h"
#include "Type.h"

namespace storm {

	Named *find(Par<NameLookup> root, Par<Name> name) {
		if (name->size() == 0)
			return as<Named>(root.borrow());

		Named *at = root->find(name->at(0));
		for (nat i = 1; at != null && i < name->size(); i++) {
			at = at->find(name->at(i));
		}

		return at;
	}

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

	void Scope::deepCopy(Par<CloneEnv> env) {
		// Should be OK to not do anything here... All our members are threaded objects.
	}

	Named *Scope::find(Par<Name> name) const {
		if (lookup)
			return lookup->find(*this, name);

		return null;
	}

	Named *find(const Scope &scope, Par<Name> name) {
		Named *r = scope.find(name);
		r->addRef();
		return r;
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
		while (p->parent()) {
			assert(as<Package>(p->parent()));
			p = (Package*)p->parent();
		}
		return p;
	}

	Package *ScopeLookup::corePkg(NameLookup *l) {
		Package *top = firstPkg(l);
		Package *root = rootPkg(top);
		Auto<NamePart> core = CREATE(NamePart, l, L"core");
		return as<Package>(root->find(core));
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

	Named *ScopeLookup::find(const Scope &in, Par<Name> name) {
		// Regular path.
		for (NameLookup *at = in.top; at; at = nextCandidate(at)) {
			if (Named *found = storm::find(at, name))
				return found;
		}

		// Core.
		if (Package *core = corePkg(in.top))
			if (Named *found = storm::find(core, name))
				return found;

		// We failed!
		return null;
	}

	/**
	 * Extra
	 */

	ScopeExtra::ScopeExtra() {}

	Named *ScopeExtra::find(const Scope &in, Par<Name> name) {
		if (Named *found = ScopeLookup::find(in, name))
			return found;

		for (nat i = 0; i < extra.size(); i++) {
			if (Named *found = storm::find(extra[i], name))
				return found;
		}

		return null;
	}

	/**
	 * Helpers.
	 */
	Scope rootScope(EnginePtr e) {
		return *e.v.scope();
	}

}
