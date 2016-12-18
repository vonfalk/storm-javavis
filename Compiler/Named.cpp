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

	Named *NameLookup::find(SimplePart *part) {
		return null;
	}

	Named *NameLookup::find(Str *name, Array<Value> *params) {
		return find(new (this) SimplePart(name, params));
	}

	Named *NameLookup::find(Str *name, Value param) {
		return find(new (this) SimplePart(name, param));
	}

	Named *NameLookup::find(Str *name) {
		return find(new (this) SimplePart(name));
	}


	Named *NameLookup::find(const wchar *name, Array<Value> *params) {
		return find(new (this) Str(name), params);
	}

	Named *NameLookup::find(const wchar *name, Value param) {
		return find(new (this) Str(name), param);
	}

	Named *NameLookup::find(const wchar *name) {
		return find(new (this) Str(name));
	}

	Named::Named(Str *name) : name(name), flags(namedDefault) {
		if (engine().has(bootTemplates)) {
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
			return new (this) SimpleName();
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

	void Named::notifyAdded(NameSet *to, Named *added) {}

	void Named::toS(StrBuf *buf) const {
		*buf << identifier();
	}

}
