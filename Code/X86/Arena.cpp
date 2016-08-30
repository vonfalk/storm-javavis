#include "stdafx.h"
#include "Arena.h"
#include "OutputX86.h"
#include "Listing.h"
#include "RemoveInvalid.h"

namespace code {
	namespace x86 {

		static bool has64(Listing *in) {
			for (nat i = 0; i < in->count(); i++) {
				if (in->at(i)->size() == Size::sLong)
					return true;
			}
			return false;
		}

		Arena::Arena() {}

		Listing *Arena::transform(Listing *l, Binary *owner) const {
			if (has64(l)) {
				// Replace any 64-bit operations with 32-bit corresponding operations.
				TODO(L"We need to transform the 64-bit ops!");
			}

			// Transform any unsupported op-codes into sequences of other op-codes. Eg. referencing
			// memory twice or similar.
			l = code::transform(l, new (this) RemoveInvalid());

			// Expand variables and function calls as well as function prolog and epilog. We need to
			// know all used registers for this to work, so it has to be run after the previous
			// transforms.

			TODO(L"Implement me!");
			return l;
		}

		void Arena::output(Listing *src, Output *to) const {
			TODO(L"Implement me!");
		}

		LabelOutput *Arena::labelOutput() const {
			return new (this) LabelOutput(4);
		}

		Output *Arena::codeOutput(Array<Nat> *offsets, Nat size) const {
			TODO(L"Implement me!");
			return null;
		}

		const Register ptrD = Register(0x010);
		const Register ptrSi = Register(0x011);
		const Register ptrDi = Register(0x012);
		const Register edx = Register(0x410);
		const Register esi = Register(0x411);
		const Register edi = Register(0x412);

		const wchar *nameX86(Register r) {
			switch (r) {
			case ptrD:
				return L"ptrD";
			case ptrSi:
				return L"ptrSi";
			case ptrDi:
				return L"ptrDi";

			case edx:
				return L"edx";
			case esi:
				return L"esi";
			case edi:
				return L"edi";
			}
			return null;
		}

		Register unusedReg(RegSet *in) {
			Register candidates[] = { ptrD, ptrA, ptrB, ptrC, ptrSi, ptrDi };
			Register protect64[] = { rax, noReg, noReg, noReg, rbx, rcx };
			for (nat i = 0; i < ARRAY_COUNT(candidates); i++) {
				if (!in->has(candidates[i])) {
					// See if we need to protect 64-bit registers...
					Register prot = protect64[i];
					if (prot == noReg)
						return candidates[i];

					// Either too small or not in the set?
					if (in->get(prot) != prot)
						return candidates[i];
				}
			}

			return noReg;
		}

		void add64(RegSet *to) {
			for (RegSet::Iter i = to->begin(); i != to->end(); ++i) {
				Register r = high32(*i);
				if (r != noReg)
					to->put(r);
			}
		}

		Register low32(Register reg) {
			if (size(reg) == Size::sLong)
				return asSize(reg, Size::sInt);
			else
				return reg;
		}

		Register high32(Register reg) {
			switch (reg) {
			case rax:
				return edx;
			case rbx:
				return esi;
			case rcx:
				return edi;
			default:
				return noReg;
			}
		}

		RegSet *allRegs(EnginePtr e) {
			RegSet *r = new (e.v) RegSet();
			r->put(eax);
			r->put(ebx);
			r->put(ecx);
			r->put(edx);
			r->put(esi);
			r->put(edi);
			return r;
		}

		RegSet *fnDirtyRegs(EnginePtr e) {
			RegSet *r = new (e.v) RegSet();
			r->put(eax);
			r->put(ecx);
			r->put(edx);
			return r;
		}

	}
}
