#pragma once
#include "Size.h"

namespace code {
	STORM_PKG(core.asm);

	/**
	 * Registers available for all backends.
	 */
	enum Register {
		// No register.
		noReg,

		// Pointer registers:
		ptrStack, // esp
		ptrFrame, // ebp

		ptrA, // return value goes here (eax)
		ptrB, // general purpose, overwritten in function calls
		ptrC, // general purpose, overwritten in function calls

		// 1 byte variants
		al = ptrA | 0x100,
		bl = ptrB | 0x100,
		cl = ptrC | 0x100,

		// 4 byte variants
		eax = ptrA | 0x400,
		ebx = ptrB | 0x400,
		ecx = ptrC | 0x400,

		// 8 byte variants
		rax = ptrA | 0x800,
		rbx = ptrB | 0x800,
		rcx = ptrC | 0x800,
	};

	// Get the name of a register.
	const wchar *name(Register r);

	// Size of registers.
	Size STORM_FN size(Register r);

	// Get the corresponding register with another size.
	Register STORM_FN asSize(Register r, Size size);
}
