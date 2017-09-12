#include "stdafx.h"
#include "Asm.h"

namespace code {
	namespace x64 {

		// Note: We intentionally overlap with the X86 implementation here.
		const Reg ptrD = Reg(0x010);
		const Reg ptrSi = Reg(0x011);
		const Reg ptrDi = Reg(0x012);
		const Reg ptr8 = Reg(0x020);
		const Reg ptr9 = Reg(0x021);
		const Reg ptr10 = Reg(0x022);
		const Reg ptr11 = Reg(0x023);
		const Reg ptr12 = Reg(0x024);
		const Reg ptr13 = Reg(0x025);
		const Reg ptr14 = Reg(0x026);
		const Reg ptr15 = Reg(0x027);
		const Reg dl = Reg(0x110);
		const Reg sil = Reg(0x111);
		const Reg dil = Reg(0x112);
		const Reg edx = Reg(0x410);
		const Reg esi = Reg(0x411);
		const Reg edi = Reg(0x412);
		const Reg rdx = Reg(0x810);
		const Reg rsi = Reg(0x811);
		const Reg rdi = Reg(0x812);
		const Reg e8 = Reg(0x420);
		const Reg e9 = Reg(0x421);
		const Reg e10 = Reg(0x422);
		const Reg e11 = Reg(0x423);
		const Reg e12 = Reg(0x424);
		const Reg e13 = Reg(0x425);
		const Reg e14 = Reg(0x426);
		const Reg e15 = Reg(0x427);
		const Reg r8 = Reg(0x820);
		const Reg r9 = Reg(0x821);
		const Reg r10 = Reg(0x822);
		const Reg r11 = Reg(0x823);
		const Reg r12 = Reg(0x824);
		const Reg r13 = Reg(0x825);
		const Reg r14 = Reg(0x826);
		const Reg r15 = Reg(0x827);

#define CASE_REG(name) case name: return S(#name)

		const wchar *nameX64(Reg r) {
			switch (r) {
				CASE_REG(ptrD);
				CASE_REG(ptrSi);
				CASE_REG(ptrDi);
				CASE_REG(ptr8);
				CASE_REG(ptr9);
				CASE_REG(ptr10);
				CASE_REG(ptr11);
				CASE_REG(ptr12);
				CASE_REG(ptr13);
				CASE_REG(ptr14);
				CASE_REG(ptr15);
				CASE_REG(dl);
				CASE_REG(sil);
				CASE_REG(dil);
				CASE_REG(edx);
				CASE_REG(esi);
				CASE_REG(edi);
				CASE_REG(rdx);
				CASE_REG(rsi);
				CASE_REG(rdi);
				CASE_REG(e8);
				CASE_REG(e9);
				CASE_REG(e10);
				CASE_REG(e11);
				CASE_REG(e12);
				CASE_REG(e13);
				CASE_REG(e14);
				CASE_REG(e15);
				CASE_REG(r8);
				CASE_REG(r9);
				CASE_REG(r10);
				CASE_REG(r11);
				CASE_REG(r12);
				CASE_REG(r13);
				CASE_REG(r14);
				CASE_REG(r15);
			default:
				return null;
			}
		}


		nat registerId(Reg r) {
			switch (asSize(r, Size::sPtr)) {
			case ptrA:
				return 0;
			case ptrC:
				return 1;
			case ptrB:
				return 3;
			case ptrStack:
				return 4;
			case ptrFrame:
				return 5;
			case ptrD:
				return 2;
			case ptrSi:
				return 6;
			case ptrDi:
				return 7;
			case ptr8:
				return 8;
			case ptr9:
				return 9;
			case ptr10:
				return 10;
			case ptr11:
				return 11;
			case ptr12:
				return 12;
			case ptr13:
				return 13;
			case ptr14:
				return 14;
			case ptr15:
				return 15;
			default:
				assert(false, L"Can not use " + ::toS(name(r)));
				return 0;
			}
		}

		bool singleByte(Word value) {
			Long v(value);
			return v >= -128 && v <= 127;
		}

		bool singleInt(Word value) {
			const Long limit = Long(1) << Long(32);
			Long v(value);
			return v >= -limit && v < limit;
		}

		Reg unusedReg(RegSet *in) {
			static const Reg candidates[] = {
				// Scratch registers:
				ptr11, ptr10, ptr9, ptr8, ptrC, ptrD, ptrSi, ptrDi, ptrA,
				// Registers that need preservation:
				ptrB, ptr12, ptr13, ptr14, ptr15
			};
			for (nat i = 0; i < ARRAY_COUNT(candidates); i++) {
				if (!in->has(candidates[i]))
					return candidates[i];
			}

			return noReg;
		}

	}
}
