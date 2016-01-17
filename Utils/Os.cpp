#include "stdafx.h"
#include "Os.h"

namespace util {

	bool isVistaOrLater() {
		OSVERSIONINFO verInfo;
		verInfo.dwOSVersionInfoSize = sizeof(verInfo);
		GetVersionEx(&verInfo);
		if (verInfo.dwMajorVersion < 6) return false;
		if (verInfo.dwMinorVersion < 0) return false;
		return true;
	}

}
