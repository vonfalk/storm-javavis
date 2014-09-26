#include "StdAfx.h"
#include "Label.h"

namespace code {

	String Label::toString() const {
		if (id == 0)
			return L"meta";
		else
			return toS(id);
	}

}