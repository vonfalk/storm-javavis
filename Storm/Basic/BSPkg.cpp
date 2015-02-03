#include "stdafx.h"
#include "BSPkg.h"

namespace storm {

	bs::Pkg::Pkg() {
		n = CREATE(Name, this);
	}

	void bs::Pkg::add(Par<SStr> part) {
		n->add(steal(CREATE(NamePart, this, part->v)));
	}

	Name *bs::Pkg::name() const {
		return n.ret();
	}

}
