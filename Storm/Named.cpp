#include "stdafx.h"
#include "Named.h"
#include "Lib/Str.h"

namespace storm {

	Named *NameLookup::find(const Name &name) {
		assert(false); // Implement me!
		return null;
	}

	NameLookup *NameLookup::parent() const {
		assert(false); // Implement me!
		return null;
	}

	Named::Named(Auto<Str> name) : name(name->v) {}

	Named *Named::closestNamed() const {
		for (NameLookup *p = parent(); p; p = p->parent()) {
			if (Named *n = as<Named>(p))
				return n;
		}
		return null;
	}

	Name Named::path() const {
		Named *parent = closestNamed();
		if (parent)
			return parent->path() + Name(name);
		else
			return Name(name);
	}

}
