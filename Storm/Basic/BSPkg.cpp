#include "stdafx.h"
#include "BSPkg.h"

namespace storm {

	bs::Pkg::Pkg() {}

	void bs::Pkg::add(Par<SStr> part) {
		parts.push_back(part->v->v);
	}

	Name bs::Pkg::name() const {
		return Name(parts);
	}

}
