#pragma once

#include "Utils/Utils.h"
#include "Utils/Platform.h"

// Use standalone stack walking (ie no external libraries). This will not
// always work well for optimized code. On windows, the default is to use the DbgHelp library.
// #define STANDALONE_STACKWALK

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

// Checks that something is defined!
#if !defined(X86) && !defined(X64)
#error "Unsupported architecture, currently supported are x86 and amd-64"
#endif

#if !defined(WINDOWS) && !defined(LINUX) && !defined(POSIX)
#error "Unknown OS, currently known are Windows and Linux"
#endif

#ifdef WINDOWS

#include <Windows.h>

// Sometimes it re-defines "small" to "char", no good!
#ifdef small
#undef small
#endif

// We need to remove min and max macros...
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

#define SEH

namespace code {

	// Use the types from the windows api headers...
	typedef UINT_PTR cpuNat;
	typedef INT_PTR cpuInt;

	typedef char Char;
	typedef unsigned char Byte;
	typedef int Int;
	typedef unsigned int Nat;
	typedef long long int Long;
	typedef unsigned long long int Word;

#define CODECALL __cdecl

}

#else

#error "Unsupported system!";

#endif

#ifndef CODECALL
#error "someone forgot to declare CODECALL for your architecture"
#endif

#include "Code/Size.h"
#include "Utils/Printable.h"
#include "Utils/Memory.h" // OFFSET_OF, BASE_PTR


