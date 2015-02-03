#include "stdafx.h"
#include "Named.h"
#include "Lib/Str.h"

namespace storm {

	NameLookup::NameLookup() : parentLookup(null) {}

	Named *NameLookup::find(Par<NamePart> name) {
		assert(false); // Implement me!
		return null;
	}

	NameLookup *NameLookup::parent() const {
		assert(parentLookup != null);
		return null;
	}

	Named::Named(Par<Str> name) : name(name->v) {}

	Named::Named(const String &name) : name(name) {}

	Named::Named(const String &name, const vector<Value> &params) : name(name), params(params) {}

	Named *Named::closestNamed() const {
		for (NameLookup *p = parent(); p; p = p->parent()) {
			if (Named *n = as<Named>(p))
				return n;
		}
		return null;
	}

	Name *Named::path() const {
		Named *parent = closestNamed();

		Name *r = null;
		if (parent)
			r = parent->path();
		else
			r = CREATE(Name, this);

		r->add(steal(CREATE(NamePart, this, name, params)));
		return r;
	}

	String Named::identifier() const {
		return ::toS(path());
	}

}
