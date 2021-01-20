// #pragma once // GCC issues a warning when using 'pragma once' with precompiled headers...
#ifndef SSL_LIB_H
#define SSL_LIB_H
#include "Shared/Storm.h"

namespace ssl {

	using namespace storm;

}

#ifdef WINDOWS

// Common includes for Windows.
#define SECURITY_WIN32
#include <Security.h>
#include <Schannel.h>

// Missing from time to time.
#ifndef SECBUFFER_ALERT
#define SECBUFFER_ALERT 17
#endif


#endif

#endif
