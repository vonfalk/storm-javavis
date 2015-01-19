#include "stdafx.h"
#include "BSIncludes.h"

namespace storm {

	bs::Includes::Includes() {}

	void bs::Includes::add(Par<Pkg> pkg) {
		names.push_back(pkg->name());
	}

}
