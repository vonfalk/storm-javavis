#include "stdafx.h"
#include "BSPkg.h"

namespace storm {

	bs::Pkg::Pkg() : Object() {}

	void bs::Pkg::add(Auto<Str> part) {
		parts.push_back(part->v);
	}

	Name bs::Pkg::name() const {
		return Name(parts);
	}

}
