#include "stdafx.h"
#include "Named.h"
#include "Engine.h"
#include "Core/Str.h"
#include "Core/StrBuf.h"
#include "Name.h"

namespace storm {

	/**
	 * NameLookup.
	 */

	NameLookup::NameLookup() : parentLookup(null) {}

	NameLookup::NameLookup(NameLookup *parent) : parentLookup(parent) {}

	NameLookup *NameLookup::parent() const {
		assert(parentLookup, L"No parent yet!");
		return parentLookup;
	}

	Named *NameLookup::find(SimplePart *part, Scope source) {
		return null;
	}


	Named *NameLookup::find(Str *name, Array<Value> *params, Scope source) {
		return find(new (this) SimplePart(name, params), source);
	}

	Named *NameLookup::find(Str *name, Value param, Scope source) {
		return find(new (this) SimplePart(name, param), source);
	}

	Named *NameLookup::find(Str *name, Scope source) {
		return find(new (this) SimplePart(name), source);
	}


	Named *NameLookup::find(const wchar *name, Array<Value> *params, Scope source) {
		return find(new (this) Str(name), params, source);
	}

	Named *NameLookup::find(const wchar *name, Value param, Scope source) {
		return find(new (this) Str(name), param, source);
	}

	Named *NameLookup::find(const wchar *name, Scope source) {
		return find(new (this) Str(name), source);
	}


	/**
	 * Named.
	 */

	Named::Named(Str *name) : name(name), flags(namedDefault), visibility(null) {
		if (engine().has(bootTemplates)) {
			params = new (this) Array<Value>();
		}
	}

	Named::Named(Str *name, Array<Value> *params) : name(name), params(params), flags(namedDefault), visibility(null) {}

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
		if (parentLookup == null)
			return shortIdentifier();

		return path()->toS();
	}

	Str *Named::shortIdentifier() const {
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

	void Named::notifyAdded(NameSet *to, Named *added) {}

	void Named::notifyRemoved(NameSet *to, Named *added) {}

	void Named::compile() {}

	void Named::toS(StrBuf *buf) const {
		*buf << identifier() << S(" [");
		putVisibility(buf);
		*buf << S("]");
	}

	void Named::putVisibility(StrBuf *to) const {
		if (visibility)
			*to << visibility;
		else
			*to << S("public (unset)");
	}

	Bool Named::visibleFrom(Scope source) {
		return visibleFrom(source.top);
	}

	Bool Named::visibleFrom(MAYBE(NameLookup *) source) {
		if (visibility && source)
			return visibility->visible(this, source);

		// If either 'visibility' or 'source' is null, we default to 'public'.
		return true;
	}

	Doc *Named::findDoc() {
		if (documentation)
			return documentation->get();
		else
			return doc(this);
	}

}
