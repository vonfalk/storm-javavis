#pragma once
#include "Utils/Utils.h"
#include "Utils/Platform.h"
#include "OS/Types.h"

// This file defines some common types in the asm generator, such as word-sized types.
// Defines one of the following symbols:
// X86 - When compiling for a 32-bit X86
// X64 - When compiling for a 64-bit amd64
// WINDOWS - When compiling for Windows
// LINUX - When compiling for Linux

// Asm types (portable):
// Char - 8-bit signed integer
// Byte - 8-bit unsigned integer
// Int - 32-bit signed integer
// Nat - 32-bit unsigned integer
// Long - 64-bit signed integer
// Word - 64-bit unsigned integer (may change)

// Internal types (not portable):
// cpuNat - unsigned pointer-sized type
// cpuInt - signed pointer-sized type

// For function calls, the following macro is defined:
// CODECALL
// Which expands to the calling convention to be used by the generated function calls.

// Thread local storage:
// THREAD, eg. THREAD int foo;
// Beware, this is not respected for UThread threads!

// Make sure the current CPU architecture is supported.
#if !defined(X86) && !defined(X64)
#error "Unsupported architecture, currently only x86 and x86-64 are supported."
#endif

#if !defined(WINDOWS) && !defined(LINUX) && !defined(POSIX)
#error "Unknown OS, currently known are Windows and Linux."
#endif

#ifdef WINDOWS

#define SEH

namespace code {

	// Use the types from the windows api headers...
	typedef UINT_PTR cpuNat;
	typedef INT_PTR cpuInt;

}

#else

#error "Unsupported system!"

#endif

// Reuse some types from OS.
namespace code {
	using os::Char;
	using os::Byte;
	using os::Int;
	using os::Nat;
	using os::Long;
	using os::Word;
	using os::Float;
}

#include "Codecall.h"
#include "Utils/Printable.h"
#include "Utils/Memory.h"
#include "Code/Size.h"
