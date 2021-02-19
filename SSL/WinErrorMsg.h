#pragma once

#ifdef WINDOWS

#define SECURITY_WIN32
#include <Security.h>

namespace ssl {

	void throwError(Engine &e, const wchar *message, SECURITY_STATUS status);

}

#endif
