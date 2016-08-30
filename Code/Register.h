#pragma once
#include "Size.h"

namespace code {
	STORM_PKG(core.asm);

	/**
	 * Registers available for all backends.
	 *
	 * Format: 0xABC where:
	 * A is the size (0 = pointer size)
	 * B is the backend id (0 = general, 1 = x86, etc)
	 * C is specific identifier to the backend
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

	/**
	 * Set of registers. Considers registers of different sizes to be the same, but keeps track of
	 * the largest integer sized register used. Byte-sizes are promoted to 32-bit sizes.
	 *
	 * ptrStack and ptrBase are not considered at all.
	 */
	class RegSet : public Object {
		STORM_CLASS;
	public:
		// Create with no registers.
		STORM_CTOR RegSet();

		// Create with a single registers.
		STORM_CTOR RegSet(Register r);

		// Array of registers -> set.
		STORM_CTOR RegSet(Array<Register> *regs);

		// Deep copy.
		void STORM_FN deepCopy(CloneEnv *env);

		// Fill with all registers except ptrStack and ptrFrame.
		void STORM_FN fill();

		// Contains a specific register?
		Bool STORM_FN has(Register r) const;

		// Add registers.
		void STORM_FN put(Register r);

		// Get the largest register seen.
		Register STORM_FN get(Register r) const;

		// Remove register.
		void STORM_FN remove(Register r);

		// Get all registers in here.
		Array<Register> *STORM_FN all() const;

		// Iterator.
		class Iter {
			STORM_VALUE;
		public:
			Iter();
			Iter(RegSet *reg);

			Bool STORM_FN operator ==(Iter o) const;
			Bool STORM_FN operator !=(Iter o) const;

			Iter &STORM_FN operator ++();
			Iter STORM_FN operator ++(int);

			Register operator *() const;
			Register STORM_FN v() const;

		private:
			RegSet *owner;
			Nat id;

			// At end?
			bool atEnd() const;
		};

		Iter STORM_FN begin() const;
		Iter STORM_FN end() const;

		// ToS.
		virtual void STORM_FN toS(StrBuf *to) const;

	private:
		enum {
			maxReg = 256
		};

		// Store largest size of each register seen. We only care about 32 or 64 bits. Stores 2 bits
		// per register. Therefore, we need 2x256 bits = 32 Words.
		// TODO: Maybe this is a bit too large. We only need about 2x64 bits right now.
		Word data;
		Word data1;
		Word data2;
		Word data3;
		Word data4;
		Word data5;
		Word data6;
		Word data7;
		Word data8;
		Word data9;
		Word data10;
		Word data11;
		Word data12;
		Word data13;
		Word data14;
		Word data15;

		// Get/set bits. 0 = not in set, 1 = 32 bit seen, 2 = pointer seen, 3 = 64-bit seen.
		Nat read(Nat id) const;
		void write(Nat id, Nat v);

		// To/from index.
		static Register toReg(Nat index, Nat size);
		static Nat toIndex(Register reg);
		static Nat toSize(Register reg);
	};
}
