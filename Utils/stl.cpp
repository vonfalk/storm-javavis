#include "stdafx.h"
#include "stl.h"

bool NoCaseLess::operator() (const String &lhs, const String &rhs) const {
	return lhs.compareNoCase(rhs) < 0;
}
