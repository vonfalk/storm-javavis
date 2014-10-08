#pragma once

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


#if defined(_WIN64)
#define X64
#define WINDOWS
#elif defined(_WIN32)
#define X86
#define WINDOWS
#endif
// TODO: More architectures here!


// Checks that something is defined!
#if !defined(X86) && !defined(X64)
#error "Unknown architecture, currently known are x86 and amd-64"
#endif

#if !defined(WINDOWS) && !defined(LINUX)
#error "Unknown OS, currently known are Windows and Linux"
#endif

// Currently unsupported checks...
#ifdef LINUX
#error "Linux is not currently supported!"
#endif

#ifdef X64
#error "amd64 is not currently supported!"
#endif

#ifdef WINDOWS

#include <Windows.h>
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
#endif


#ifndef CODECALL
#error "someone forgot to declare CODECALL for your architecture"
#endif

#include "Utils/Printable.h"

// Get the offset of a specific variable inside a struct.
#define OFFSET_OF(type, member) size_t(&((type *)0)->member)
