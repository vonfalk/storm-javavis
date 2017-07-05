#pragma once

#include "Utils/Platform.h"

namespace os {

	/**
	 * Defines platform independed types:
	 * Char -  8-bit signed   integer
	 * Byte -  8-bit unsidned integer
	 * Int  - 32-bit signed   integer
	 * Nat  - 32-bit unsigned integer
	 * Long - 64-bit signed   integer
	 * Word - 64-bit unsigned integer
	 *
	 * Float - 32-bit floating point
	 */

#if defined(WINDOWS)
	typedef char Char;
	typedef unsigned char Byte;
	typedef int Int;
	typedef unsigned int Nat;
	typedef long long int Long;
	typedef unsigned long long int Word;
	typedef float Float;
#elif defined(POSIX)
	typedef char Char;
	typedef unsigned char Byte;
	typedef int32_t Int;
	typedef uint32_t Nat;
	typedef int64_t Long;
	typedef uint64_t Word;
	typedef float Float;
#else
#error "Unsupported system. Please define types for your compiler/system!"
#endif

}
