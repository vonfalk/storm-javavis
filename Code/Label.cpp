#include "StdAfx.h"
#include "Label.h"

namespace code {

	String Label::toS() const {
		if (id == 0)
			return L"meta";
		else
			return ::toS(id);
	}

}
