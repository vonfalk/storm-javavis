#pragma once
#include "Size.h"
#include "Core/Object.h"
#include "Core/Array.h"

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

	// Are the two registers the same, disregarding size?
	Bool STORM_FN same(Register a, Register b);

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

		// Copy.
		STORM_CTOR RegSet(const RegSet *src);

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
		void STORM_FN put(const RegSet *r);

		// Get the largest register seen.
		Register STORM_FN get(Register r) const;

		// Remove register.
		void STORM_FN remove(Register r);

		// Clear.
		void STORM_FN clear();

		// Get all registers in here.
		Array<Register> *STORM_FN all() const;

		// Iterator.
		class Iter {
			STORM_VALUE;
		public:
			Iter();
			Iter(const RegSet *reg);

			Bool STORM_FN operator ==(Iter o) const;
			Bool STORM_FN operator !=(Iter o) const;

			Iter &STORM_FN operator ++();
			Iter STORM_FN operator ++(int z);

			Register operator *() const;
			Register STORM_FN v() const;

		private:
			const RegSet *owner;

			// Pos: lower 4 bits = slot, rest = bank.
			Nat pos;

			// At end?
			bool atEnd() const;

			// Empty at pos?
			bool empty(Nat pos) const;
			Register read(Nat pos) const;
		};

		Iter STORM_FN begin() const;
		Iter STORM_FN end() const;

		// ToS.
		virtual void STORM_FN toS(StrBuf *to) const;

	private:
		/**
		 * Data storage. We store 'banks'*16 entries here as follows:
		 *
		 * 'index' stores the backed id for the 'dataX' segments. 'data0' always has backend id = 0,
		 * as those are commonly used. 'index' stores four bits for each 'dataX', zero means free.
		 *
		 * Each of 'dataX' stores two bits for each entry inside it. Each entry represents one
		 * possible value of the last four bits of a Register. These two bits have the following
		 * four values:
		 *
		 * 0: not in the set
		 * 1: 32 bit value is in the set
		 * 2: pointer is in the set
		 * 3: 64 bit value is in the set
		 *
		 * We only need this few registers as it is uncommon to mix registers from different
		 * backends. If some backends require larger storage in the future, this scheme is easily
		 * expandable.
		 */

		// Constants:
		enum {
			// 2 banks is enough for now. Make sure to add 'data' entries up to the number of banks used.
			// At least 2, maximum 9.
			banks = 2,

			// Slots per data entry. Should be 16.
			dataSlots = 8 * sizeof(Nat) / 2,
		};

		// Index. Note that data0 is excluded from the index as that always represents the common
		// registers above.
		Nat index;

		// Data. Make sure to have at least 'banks' of these.
		Nat data0;
		Nat data1;

		/**
		 * Low-level access to the data. An id points into one two-bit position inside data (with an
		 * associated index location).
		 */

		// Read/write index.
		Nat readIndex(Nat bank) const;
		void writeIndex(Nat bank, Nat v);

		// Read/write data.
		Nat readData(Nat bank, Nat slot) const;
		void writeData(Nat bank, Nat slot, Nat v);

		// Bank manipulation.
		bool emptyBank(Nat bank) const;
		Nat findBank(Nat backendId) const;
		Nat allocBank(Nat backendId);

		// Read a register.
		Register readRegister(Nat bank, Nat slot) const;

		/**
		 * Register manipulation helpers.
		 */

		// Get the slot id of a register.
		static Nat registerSlot(Register r);
		static Nat registerBackend(Register r);
		static Nat registerSize(Register r);

	};
}
