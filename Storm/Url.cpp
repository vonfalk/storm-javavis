#include "stdafx.h"
#include "Url.h"
#include "Engine.h"
#include "Package.h"

namespace storm {

	Url *rootUrl(EnginePtr e) {
		Url *u = e.v.rootPackage()->url();
		if (u)
			return u;

		return executableUrl(e.v);
	}

}
