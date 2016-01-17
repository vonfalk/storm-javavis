#include "stdafx.h"
#include "stl.h"

bool NoCaseLess::operator() (const String &lhs, const String &rhs) const {
	return _wcsicmp(lhs.c_str(), rhs.c_str()) < 0;
}
