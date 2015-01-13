#pragma once
#include "Size.h"

// Defines the available registers to the end-user of this API. These registers
// are not to be overwritten by using ASM-instructions unless they are specified
// as a dest parameter of an instruction.

namespace code {

	// Backends may define other registers with IDs above 0xFF.
	enum Register {
		noReg, // No register

		// Pointer registers.
		ptrStack, // esp
		ptrFrame, // ebp

		ptrA, // return value goes here (eax)
		ptrB, // general purpose (overwritten in function calls)
		ptrC, // general purpose (overwritten in function calls)

		// 1 byte variants
		al = ptrA | 0x10,
		bl = ptrB | 0x10,
		cl = ptrC | 0x10,

		// 4 byte variants
		eax = ptrA | 0x40,
		ebx = ptrB | 0x40,
		ecx = ptrC | 0x40,

		// 8 byte variants
		rax = ptrA | 0x80,
		rbx = ptrB | 0x80,
		rcx = ptrC | 0x80,
	};

	bool fromBackend(Register r);
	const wchar_t *name(Register r);

	// nat size(Register r);
	Size size(Register r);
	Register asSize(Register r, nat size=0);

	// Returns 'noReg' if the desired size is not available.
	Register asSize(Register r, Size size);


	// Collection of registers. Considers registers of different sizes as the same register, but keeps track
	// of the different sizes used. The -= operator erases all sizes of the register.
	class Registers {
	public:
		// Create with no registers.
		Registers();

		// Create representing a single register.
		explicit Registers(Register r);

		// Vector of registers => Registers.
		explicit Registers(const vector<Register> &r);

		// Create with all registers, except for the ptrStack and ptrFrame.
		static Registers all();

		// Iterator type.
		typedef set<Register>::const_iterator iterator;
		typedef set<Register>::const_reverse_iterator riterator;

		// Begin and end
		inline iterator begin() const { return regs.begin(); }
		inline iterator end() const { return regs.end(); }
		inline riterator rbegin() const { return regs.rbegin(); }
		inline riterator rend() const { return regs.rend(); }

		// Contains?
		bool contains(Register r) const;

		// Returns the largest variant of 'r'. NOTE: platform specific, since we have to know the size of a pointer.
		Register largest(Register r) const;

		// To string.
		String toS() const;

		// Modify members.
		Registers &operator +=(Register r);
		Registers &operator +=(const Registers &r);

		Registers &operator -=(Register r);
		Registers &operator -=(const Registers &r);

		Registers &operator &=(const Registers &r);
	private:
		// Data
		set<Register> regs;
	};
}
