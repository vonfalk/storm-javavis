#pragma once
#include "../Reg.h"

namespace code {
	namespace x64 {
		STORM_PKG(core.asm.x64);

		/**
		 * X86-64 specific registers.
		 *
		 * TODO: Expose to Storm somehow.
		 */
		extern const Reg ptrD;
		extern const Reg ptrSi;
		extern const Reg ptrDi;
		extern const Reg ptr8;
		extern const Reg ptr9;
		extern const Reg ptr10;
		extern const Reg ptr11;
		extern const Reg ptr12;
		extern const Reg ptr13;
		extern const Reg ptr14;
		extern const Reg ptr15;
		extern const Reg dl;
		extern const Reg sil;
		extern const Reg dil;
		extern const Reg edx;
		extern const Reg esi;
		extern const Reg edi;
		extern const Reg rdx;
		extern const Reg rsi;
		extern const Reg rdi;
		extern const Reg e8;
		extern const Reg e9;
		extern const Reg e10;
		extern const Reg e11;
		extern const Reg e12;
		extern const Reg e13;
		extern const Reg e14;
		extern const Reg e15;
		extern const Reg r8;
		extern const Reg r9;
		extern const Reg r10;
		extern const Reg r11;
		extern const Reg r12;
		extern const Reg r13;
		extern const Reg r14;
		extern const Reg r15;

		// Convert to names.
		const wchar *nameX64(Reg r);

		// Register ID.
		nat registerId(Reg r);

		// Does 'value' fit in a single byte?
		bool singleByte(Word value);

		// Does 'value' fit in a 32-bit word?
		bool singleInt(Word value);

		// Find a unused register given a set of used registers.
		Reg unusedReg(RegSet *in);

	}
}
