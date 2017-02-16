#pragma once

/**
 * This file contains macros for finding out which platform
 * we are currently being compiled on. This file is designed
 * to not depend on anything, and can therefore safely be included
 * from anywhere.
 */

/**
 * Machine type defines:
 * X86 - x86 cpu
 * X64 - x86-64/amd64 cpu
 */

/**
 * Platform defined:
 * LINUX - compiled on linux
 * POSIX - compiled on posix-compatible platform (eg linux)
 * WINDOWS - compiled on windows
 */

/**
 * Endian-ness: Look in Endian.h for more helpers regarding endianness.
 * LITTLE_ENDIAN
 * BIG_ENDIAN
 */


/**
 * Compilers:
 * VISUAL_STUDIO - Visual Studio compiler. Set to the version, eg 2008 for VS2008.
 * GCC - GCC compiler, TODO: version?
 */

/**
 * Standardized function/variable specifications:
 * THREAD - thread local variable
 * NAKED - naked function call, ie no prolog/epilog. Maybe not supported for all compilers.
 */

/**
 * CODECALL:
 * Calling convention used in the Code lib. This is not inside the Code lib, since it will make
 * mymake think that Code is dependent on almost all projects, since they only need the CODECALL macro.
 */


// Detect the current architecture and platform.
#if defined(_WIN64)
#define X64
#define WINDOWS
#elif defined(_WIN32)
#define X86
#define WINDOWS
#else
#error "Unknown platform, please add it here!"
#endif

// Detect the current compiler.
#if defined(_MSC_VER)
// Visual Studio compiler!
#if _MSC_VER >= 1800
#define VISUAL_STUDIO 2013
#elif _MSC_VER >= 1700
#define VISUAL_STUDIO 2012
#elif _MSC_VER >= 1600
#define VISUAL_STUDIO 2010
#elif _MSC_VER >= 1500
#define VISUAL_STUDIO 2008
#else
#error "Too early VS version, earliest supported is VS2008"
#endif

#elif defined(__GNUC__)
// GCC
#define GCC __GNUC__

#endif

#ifdef VISUAL_STUDIO
#define THREAD __declspec(thread)
#define NAKED __declspec(naked)
#define SHARED_EXPORT __declspec(dllexport)
#endif

#if defined(X86) || defined(X64)
#define LITTLE_ENDIAN
#else
#error "Unknown endianness for your platform. Define either LITTLE_ENDIAN or BIG_ENDIAN here."
#endif

#ifndef THREAD
#error "someone forgot to declare THREAD for your architecture"
#endif

#ifndef NAKED
#error "someone forgot to declare NAKED for your architecture"
#endif

#ifndef SHARED_EXPORT
#error "someone forgot to declare SHARED_EXPORT for your architecture"
#endif

#ifdef WINDOWS
#define CODECALL __cdecl
#endif

// Make sure it is defined.
#ifndef CODECALL
#error "Someone forgot to declare CODECALL for your architecture."
#endif
