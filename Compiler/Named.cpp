#include "stdafx.h"
#include "Named.h"
#include "Str.h"
#include "StrBuf.h"

namespace storm {

	NameLookup::NameLookup() : parentLookup(null) {}

	NameLookup *NameLookup::parent() const {
		assert(parentLookup, L"No parent yet!");
		return parentLookup;
	}


	Named::Named(Str *name) : name(name) {}

	NameLookup *Named::parent() const {
		assert(parentLookup, L"No parent for " + ::toS(identifier()));
		return parentLookup;
	}

	Str *Named::identifier() const {
		return new (this) Str(L"TODO: Implement Named::identifier()");
	}

	void Named::toS(StrBuf *buf) const {
		*buf << identifier();
	}

}
