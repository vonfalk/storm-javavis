#include "stdafx.h"
#include "Arena.h"
#include "OutputX86.h"
#include "Listing.h"
#include "Layout.h"
#include "Remove64.h"
#include "RemoveInvalid.h"
#include "LayoutVars.h"

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
				l = code::transform(l, new (this) Remove64());
			}

			// Transform any unsupported op-codes into sequences of other op-codes. Eg. referencing
			// memory twice or similar.
			l = code::transform(l, new (this) RemoveInvalid());

			// Expand variables and function calls as well as function prolog and epilog. We need to
			// know all used registers for this to work, so it has to be run after the previous
			// transforms.
			l = code::transform(l, new (this) LayoutVars());

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

		Operand low32(Operand o) {
			assert(o.size() == Size::sLong);
			switch (o.type()) {
			case opConstant:
				return natConst(o.constant() & 0xFFFFFFFF);
			case opRegister:
				return Operand(low32(o.reg()));
			case opRelative:
				return intRel(o.reg(), o.offset());
			case opVariable:
				return intRel(o.variable(), o.offset());
			}
			assert(false);
			return Operand();
		}

		Operand high32(Operand o) {
			assert(o.size() == Size::sLong);
			switch (o.type()) {
			case opConstant:
				return natConst(o.constant() >> 32);
			case opRegister:
				return Operand(high32(o.reg()));
			case opRelative:
				return intRel(o.reg(), o.offset() + Offset(4));
			case opVariable:
				return intRel(o.variable(), o.offset() + Offset(4));
			}
			assert(false);
			return Operand();
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

		Register preserve(Register r, RegSet *used, Listing *dest) {
			Engine &e = dest->engine();
			Register into = unusedReg(used);
			if (into == noReg) {
				*dest << push(e, r);
			} else {
				into = asSize(into, size(r));
				*dest << mov(e, into, r);
			}
			return into;
		}

		void restore(Register r, Register saved, Listing *dest) {
			Engine &e = dest->engine();

			if (saved == noReg) {
				*dest << pop(e, r);
			} else {
				*dest << mov(e, r, saved);
			}
		}

		Preserve::Preserve(RegSet *regs, RegSet *used, Listing *dest) {
			this->dest = dest;
			srcReg = new (dest) Array<Nat>();
			destReg = new (dest) Array<Nat>();
			RegSet *usedBefore = used;
			RegSet *usedAfter = new (used) RegSet(used);
			add64(usedAfter);

			for (RegSet::Iter i = regs->begin(); i != regs->end(); ++i) {
				if (usedBefore->has(*i)) {
					Register r = preserve(*i, usedAfter, dest);
					srcReg->push(Nat(*i));
					destReg->push(Nat(r));
				}
			}
		}

		void Preserve::restore() {
			for (Nat i = srcReg->count(); i > 0; i--) {
				Register src = Register(srcReg->at(i - 1));
				Register dest = Register(destReg->at(i - 1));
				code::x86::restore(src, dest, this->dest);
			}
		}

		static Offset paramOffset(Listing *src, Variable var) {
			if (var == Variable()) {
				// Old ebp and return pointer.
				return Offset::sPtr * 2;
			}

			Variable prev = src->prev(var);
			Offset offset = paramOffset(src, prev) + prev.size();
			return offset.alignAs(Size::sPtr);
		}

		Array<Offset> *layout(Listing *src, Nat savedRegs, Bool usingEH) {
			Array<Offset> *result = code::layout(src);
			Array<Variable> *all = src->allVars();

			Offset varOffset;
			// Old ebp.
			varOffset += Size::sPtr;
			// Exception handler frame.
			if (usingEH)
				varOffset += Size::sPtr * 4;
			// Saved registers.
			varOffset += Size::sPtr * savedRegs;

			for (nat i = 0; i < all->count(); i++) {
				Variable var = all->at(i);
				Nat id = var.key();

				if (src->isParam(var)) {
					result->at(id) = paramOffset(src, var);
				} else {
					result->at(id) = -(result->at(id) + varOffset);
				}
			}

			result->last() = -(result->last() + varOffset);
			return result;
		}

	}
}
