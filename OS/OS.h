#pragma once
#include "Utils/Utils.h"
#include "Utils/Platform.h"

// Make sure the current CPU architecture is supported.
#if !defined(X86) && !defined(X64)
#error "Unsupported architecture, currently only x86 and x86-64 are supported."
#endif

#include "Utils/Printable.h"
#include "Utils/Memory.h"
