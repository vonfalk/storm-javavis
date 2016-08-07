#include "stdafx.h"
#include "Named.h"
#include "Engine.h"
#include "Core/Str.h"
#include "Core/StrBuf.h"
#include "Name.h"

namespace storm {

	NameLookup::NameLookup() : parentLookup(null) {}

	NameLookup *NameLookup::parent() const {
		assert(parentLookup, L"No parent yet!");
		return parentLookup;
	}


	Named::Named(Str *name) : name(name), flags(namedDefault) {
		if (engine().boot() >= bootTemplates) {
			params = new (this) Array<Value>();
		}
	}

	Named::Named(Str *name, Array<Value> *params) : name(name), params(params), flags(namedDefault) {}

	void Named::lateInit() {
		if (!params)
			params = new (this) Array<Value>();
	}

	NameLookup *Named::parent() const {
		assert(parentLookup, L"No parent for " + ::toS(identifier()));
		return parentLookup;
	}

	Named *Named::closestNamed() const {
		for (NameLookup *p = parent(); p; p = p->parent()) {
			if (Named *n = as<Named>(p))
				return n;
		}
		return null;
	}

	SimpleName *Named::path() const {
		if (Named *parent = closestNamed()) {
			SimpleName *p = parent->path();
			p->add(new (this) SimplePart(name, params));
			return p;
		} else {
			return new (this) SimpleName(name, params);
		}
	}

	Str *Named::identifier() const {
		if (parentLookup == null) {
			StrBuf *out = new (this) StrBuf();
			*out << name;
			if (params != null && params->count() > 0) {
				*out << L"(" << params->at(0);
				for (nat i = 1; i < params->count(); i++)
					*out << L", " << params->at(i);
				*out << L")";
			}
			return out->toS();
		}

		return path()->toS();
	}

	void Named::toS(StrBuf *buf) const {
		*buf << identifier();
	}

}
