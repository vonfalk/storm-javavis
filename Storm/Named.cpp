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

}
