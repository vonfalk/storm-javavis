#pragma once
#include "Utils/Platform.h"

#if defined(X86) && defined(SEH)

extern "C" void __stdcall x86SafeSEH();

#endif

