#include "stdafx.h"
#include "BSIncludes.h"

namespace storm {

	bs::Includes::Includes() {}

	void bs::Includes::add(Auto<Pkg> pkg) {
		names.push_back(pkg->name());
	}

}
