// #pragma once // GCC issues a warning when using 'pragma once' with precompiled headers...
#ifndef TEST_LIB_H
#define TEST_LIB_H

#ifdef __cplusplus

#include "Shared/Storm.h"

namespace sql {

	using namespace storm;

}

#endif

#endif
