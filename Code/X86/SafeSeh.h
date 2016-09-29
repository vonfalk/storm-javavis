#pragma once
#include "Utils/Platform.h"

#if defined(WINDOWS) && defined(X86)

// Note: this declaration is a lie. Do not call directly.
extern "C"
void __stdcall x86SafeSEH();

#endif
