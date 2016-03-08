#include "stdafx.h"
#include "Named.h"
#include "Shared/Str.h"
#include "Type.h"

namespace storm {

	NameLookup::NameLookup() : parentLookup(null) {}

	Named *NameLookup::findCpp(const String &name, const vector<Value> &params) {
		return find(steal(CREATE(SimplePart, this, name, params)));
	}

	Named *NameLookup::find(Par<SimplePart> part) {
		return null;
	}

	NameLookup *NameLookup::parent() const {
		assert(parentLookup);
		return parentLookup;
	}

	void NameLookup::setParent(Par<NameLookup> lookup) {
		parentLookup = lookup.borrow();
	}


	Named::Named(Par<Str> name) : name(name->v), flags(namedDefault) {}

	static vector<Value> toVec(Par<Array<Value>> p) {
		vector<Value> r(p->count());
		for (nat i = 0; i < p->count(); i++)
			r[i] = p->at(i);
		return r;
	}

	Named::Named(Par<Str> name, Par<Array<Value>> p) : name(name->v), params(toVec(p)), flags(namedDefault) {}

	Named::Named(const String &name) : name(name), flags(namedDefault) {}

	Named::Named(const String &name, const vector<Value> &params) : name(name), params(params), flags(namedDefault) {}

	Named *Named::closestNamed() const {
		for (NameLookup *p = parent(); p; p = p->parent()) {
			if (Named *n = as<Named>(p))
				return n;
		}
		return null;
	}

	SimpleName *Named::path() const {
		Named *parent = closestNamed();

		SimpleName *r = null;
		if (parent) {
			r = parent->path();
			r->add(steal(CREATE(SimplePart, this, name, params)));
		} else {
			r = CREATE(SimpleName, this);
		}

		return r;
	}

	String Named::identifier() const {
		// Avoid crashes:
		if (parentLookup == null) {
			std::wostringstream to;
			to << name;
			if (params.size() > 0) {
				to << L"(";
				join(to, params, L", ");
				to << L")";
			}
			return to.str();
		}
		Auto<SimpleName> p = path();
		return ::toS(p);
	}

	NameLookup *Named::parent() const {
		assert(parentLookup, L"No parent for " + identifier());
		return parentLookup;
	}

	void Named::compile() {
		// Nothing to compile here.
	}

	Str *name(Par<Named> named) {
		return CREATE(Str, named, named->name);
	}

	Str *identifier(Par<Named> named) {
		return CREATE(Str, named, named->identifier());
	}

	Array<Value> *params(Par<Named> named) {
		Auto<Array<Value>> r = CREATE(Array<Value>, named);
		for (nat i = 0; i < named->params.size(); i++)
			r->push(named->params[i]);
		return r.ret();
	}

}
