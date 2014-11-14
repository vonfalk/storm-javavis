#include "stdafx.h"
#include "BSIncludes.h"

namespace storm {

	bs::Includes::Includes() : Object() {}

	void bs::Includes::add(Pkg *pkg) {
		names.push_back(pkg->name());
	}

}
