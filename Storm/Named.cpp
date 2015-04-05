#include "stdafx.h"
#include "Named.h"
#include "Lib/Str.h"
#include "Type.h"

namespace storm {

	NameLookup::NameLookup() : parentLookup(null) {}

	Named *NameLookup::findHere(const String &name, const vector<Value> &params) {
		PLN("Implement findHere on " << myType->identifier());
		assert(false); // Implement me!
		return null;
	}

	NameLookup *NameLookup::parent() const {
		assert(parentLookup);
		return parentLookup;
	}

	Named::Named(Par<Str> name) : name(name->v), matchFlags(matchDefault) {}

	static vector<Value> toVec(Par<Array<Value>> p) {
		vector<Value> r(p->count());
		for (nat i = 0; i < p->count(); i++)
			r[i] = p->at(i);
		return r;
	}

	Named::Named(Par<Str> name, Par<Array<Value>> p) : name(name->v), params(toVec(p)), matchFlags(matchDefault) {}

	Named::Named(const String &name) : name(name), matchFlags(matchDefault) {}

	Named::Named(const String &name, const vector<Value> &params) : name(name), params(params), matchFlags(matchDefault) {}

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
		if (parent) {
			r = parent->path();
			r->add(steal(CREATE(NamePart, this, name, params)));
		} else {
			r = CREATE(Name, this);
		}

		return r;
	}

	String Named::identifier() const {
		// Avoid crashes:
		if (parentLookup == null)
			return name;
		Auto<Name> p = path();
		return ::toS(p);
	}

}
