#include "stdafx.h"
#include "BSIncludes.h"

namespace storm {

	bs::Includes::Includes() {}

	void bs::Includes::add(Par<TypeName> pkg) {
		names.push_back(pkg);
	}

}
