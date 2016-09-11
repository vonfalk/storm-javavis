#include "stdafx.h"
#include "RemoveInvalid.h"
#include "Listing.h"
#include "Asm.h"
#include "Utils/Bitwise.h"

namespace code {
	namespace x86 {

#define IMM_REG(x) { op::x, &RemoveInvalid::immRegTfm }
#define TRANSFORM(x) { op::x, &RemoveInvalid::x ## Tfm }

		const OpEntry<RemoveInvalid::TransformFn> RemoveInvalid::transformMap[] = {
			IMM_REG(mov),
			IMM_REG(add),
			IMM_REG(adc),
			IMM_REG(or),
			IMM_REG(and),
			IMM_REG(sub),
			IMM_REG(sbb),
			IMM_REG(xor),
			IMM_REG(cmp),

			TRANSFORM(lea),
			TRANSFORM(mul),
			TRANSFORM(idiv),
			TRANSFORM(udiv),
			TRANSFORM(imod),
			TRANSFORM(umod),
			TRANSFORM(setCond),
			TRANSFORM(shl),
			TRANSFORM(shr),
			TRANSFORM(sar),
			TRANSFORM(icast),
			TRANSFORM(ucast),

			TRANSFORM(callFloat),
			TRANSFORM(retFloat),
			TRANSFORM(fnCallFloat),

			TRANSFORM(fnParam),
			TRANSFORM(fnParamRef),
			TRANSFORM(fnCall),
		};

		RemoveInvalid::Param::Param(Operand src, Operand copyFn) : src(src), copyFn(copyFn) {}

		RemoveInvalid::RemoveInvalid() {}

		void RemoveInvalid::before(Listing *dest, Listing *src) {
			params = new (this) Array<Param>();

			used = usedRegisters(src).used;

			// Add 64-bit aliases everywhere.
			for (nat i = 0; i < used->count(); i++)
				add64(used->at(i));
		}

		void RemoveInvalid::during(Listing *dest, Listing *src, Nat line) {
			static OpTable<TransformFn> t(transformMap, ARRAY_COUNT(transformMap));

			Instr *i = src->at(line);
			TransformFn f = t[i->op()];
			if (f) {
				(this->*f)(dest, i, line);
			} else {
				*dest << i;
			}
		}

		Register RemoveInvalid::unusedReg(Nat line) {
			return code::x86::unusedReg(used->at(line));
		}

		// ImmReg combination already supported?
		static bool supported(Instr *instr) {
			switch (instr->src().type()) {
			case opLabel:
			case opReference:
			case opConstant:
			case opRegister:
				return true;
			default:
				if (instr->dest().type() == opRegister)
					return true;
				break;
			}

			return false;
		}

		void RemoveInvalid::immRegTfm(Listing *dest, Instr *instr, Nat line) {

			if (supported(instr)) {
				*dest << instr;
				return;
			}

			Size size = instr->src().size();
			assert(size <= Size::sInt, "The 64-bit transform should have fixed this!");

			Register reg = unusedReg(line);
			Engine &e = engine();
			if (reg == noReg) {
				reg = asSize(ptrD, size);
				*dest << push(ptrD);
				*dest << mov(reg, instr->src());
				*dest << instr->alterSrc(reg);
				*dest << pop(ptrD);
			} else {
				reg = asSize(reg, size);
				*dest << mov(reg, instr->src());
				*dest << instr->alterSrc(reg);
			}
		}

		void RemoveInvalid::leaTfm(Listing *dest, Instr *instr, Nat line) {

			// We can encode writing directly to a register.
			if (instr->dest().type() == opRegister) {
				*dest << instr;
				return;
			}

			Register reg = unusedReg(line);
			Engine &e = engine();
			if (reg == noReg) {
				*dest << push(ptrD);
				*dest << lea(ptrD, instr->src());
				*dest << mov(instr->dest(), ptrD);
				*dest << pop(ptrD);
			} else {
				reg = asSize(reg, Size::sPtr);
				*dest << lea(reg, instr->src());
				*dest << mov(instr->dest(), reg);
			}
		}

		void RemoveInvalid::mulTfm(Listing *dest, Instr *instr, Nat line) {

			Size size = instr->size();
			assert(size != Size::sByte && size <= Size::sInt, "Bytes not supported yet!");

			if (instr->dest().type() == opRegister) {
				*dest << instr;
				return;
			}

			// Only supported mmode is mul <reg>, <r/m>. Move dest into a register.
			Engine &e = engine();
			Register reg = unusedReg(line);
			if (reg == noReg) {
				reg = asSize(ptrD, size);
				*dest << push(ptrD);
				*dest << mov(reg, instr->dest());
				*dest << instr->alterDest(reg);
				*dest << mov(instr->dest(), reg);
				*dest << pop(ptrD);
			} else {
				reg = asSize(reg, size);
				*dest << mov(reg, instr->dest());
				*dest << instr->alterDest(reg);
				*dest << mov(instr->dest(), reg);
			}
		}

		void RemoveInvalid::idivTfm(Listing *to, Instr *instr, Nat line) {

			Engine &e = engine();
			Operand dest = instr->dest();
			bool srcConst = instr->src().type() == opConstant;
			bool destEax = false;

			if (dest.type() == opRegister && same(dest.reg(), ptrA)) {
				destEax = true;

				if (!srcConst) {
					// Supported!
					*to << instr;
					return;
				}
			}

			// The 64-bit transform has been executed before, so we are sure that size is <= sInt
			bool isByte = dest.size() == Size::sByte;
			Operand newSrc = instr->src();

			RegSet *used = this->used->at(line);

			// Clear eax and edx.
			if (!destEax && used->has(eax))
				*to << push(eax);
			if (!isByte && used->has(edx))
				*to << push(edx);

			// Move dest into eax first.
			*to << mov(eax, dest);

			if (srcConst) {
				if (used->has(ebx))
					*to << push(ebx);
				*to << mov(ebx, instr->src());
				newSrc = ebx;
			}

			// Clear edx.
			*to << xor(edx, edx);
			*to << instr->alter(eax, newSrc);
			*to << mov(dest, eax);

			if (srcConst && used->has(ebx))
				*to << pop(ebx);
			if (!isByte && used->has(edx))
				*to << pop(edx);
			if (!destEax && used->has(eax))
				*to << pop(eax);
		}

		void RemoveInvalid::udivTfm(Listing *dest, Instr *instr, Nat line) {
			idivTfm(dest, instr, line);
		}

		void RemoveInvalid::imodTfm(Listing *to, Instr *instr, Nat line) {

			Engine &e = engine();
			Operand dest = instr->dest();
			bool srcConst = instr->src().type() == opConstant;
			bool eaxDest = dest.type() == opRegister && same(dest.reg(), ptrA);
			bool isByte = dest.size() == Size::sByte;

			Operand newSrc = instr->src();
			RegSet *used = this->used->at(line);

			// Clear eax and edx if needed.
			if (!eaxDest && used->has(eax))
				*to << push(eax);
			if (!isByte && used->has(edx))
				*to << push(edx);

			// Move source into eax.
			*to << mov(eax, dest);

			if (srcConst) {
				if (used->has(ebx))
					*to << push(ebx);
				*to << mov(ebx, instr->src());
				newSrc = ebx;
			}

			// Clear edx.
			*to << xor(edx, edx);
			*to << instr->alter(eax, newSrc);
			*to << mov(dest, edx);

			if (srcConst && used->has(ebx))
				*to << pop(ebx);
			if (!isByte && used->has(edx))
				*to << pop(edx);
			if (!eaxDest && used->has(eax))
				*to << pop(eax);
		}

		void RemoveInvalid::umodTfm(Listing *dest, Instr *instr, Nat line) {
			imodTfm(dest, instr, line);
		}

		void RemoveInvalid::setCondTfm(Listing *dest, Instr *instr, Nat line) {

			switch (instr->src().condFlag()) {
			case ifAlways:
				*dest << mov(engine(), instr->dest(), byteConst(1));
				break;
			case ifNever:
				*dest << mov(engine(), instr->dest(), byteConst(0));
				break;
			default:
				*dest << instr;
				break;
			}
		}

		void RemoveInvalid::shlTfm(Listing *dest, Instr *instr, Nat line) {
			Engine &e = engine();

			switch (instr->src().type()) {
			case opRegister:
				if (instr->src().reg() == cl) {
					*dest << instr;
					return;
				}
				break;
			case opConstant:
				// Supported!
				*dest << instr;
				return;
			}

			Size size = instr->dest().size();

			// We need to store the value in cl. See if dest is also cl or ecx:
			if (instr->dest().type() == opRegister && same(instr->dest().reg(), ecx)) {
				// Yup. We need to swap things around a lot!
				Register reg = asSize(unusedReg(line), size);

				if (reg == noReg) {
					// Ugh... Worst case!
					*dest << push(ecx);
					*dest << mov(cl, instr->src());
					*dest << instr->alter(xRel(size, ptrStack, Offset(0)), cl);
					*dest << pop(ecx);
				} else {
					*dest << mov(reg, instr->dest());
					*dest << mov(cl, instr->src());
					*dest << instr->alter(reg, cl);
					*dest << mov(instr->dest(), reg);
				}
			} else {
				// We have a bit more leeway at least!
				Register reg = asSize(unusedReg(line), Size::sInt);

				if (reg == noReg) {
					*dest << push(ecx);
					*dest << mov(cl, instr->src());
					*dest << instr->alterSrc(cl);
					*dest << pop(ecx);
				} else {
					*dest << mov(reg, ecx);
					*dest << mov(cl, instr->src());
					*dest << instr->alterSrc(cl);
					*dest << mov(ecx, reg);
				}
			}
		}

		void RemoveInvalid::shrTfm(Listing *dest, Instr *instr, Nat line) {
			shlTfm(dest, instr, line);
		}

		void RemoveInvalid::sarTfm(Listing *dest, Instr *instr, Nat line) {
			shlTfm(dest, instr, line);
		}

		void RemoveInvalid::icastTfm(Listing *dest, Instr *instr, Nat line) {
			Size sFrom = instr->src().size();
			Size sTo = instr->dest().size();
			Engine &e = engine();

			if (instr->dest() == Operand(asSize(eax, sTo))) {
				*dest << instr;
				return;
			}

			bool toEax = instr->dest().type() != opRegister || !same(instr->dest().reg(), eax);

			RegSet *used = this->used->at(line);
			bool saveEax = used->has(eax);
			bool saveEdx = used->has(edx);

			if (toEax)
				saveEax = false;
			if (sFrom != Size::sLong && sTo != Size::sLong)
				saveEdx = false;

			if (saveEax)
				*dest << push(eax);
			if (saveEdx)
				*dest << push(edx);

			if ((sFrom == Size::sByte && sTo == Size::sLong) ||
				(sFrom == Size::sLong && sTo == Size::sByte)) {
				*dest << instr->alterDest(eax);
				*dest << instr->alter(asSize(eax, sTo), eax);
			} else {
				*dest << instr->alterDest(asSize(eax, sTo));
			}

			if (!toEax) {
				if (sTo == Size::sLong) {
					*dest << mov(low32(instr->dest()), eax);
					*dest << mov(high32(instr->dest()), edx);
				} else {
					*dest << mov(instr->dest(), asSize(eax, sTo));
				}
			}

			if (saveEdx)
				*dest << pop(edx);
			if (saveEax)
				*dest << pop(edx);
		}

		void RemoveInvalid::ucastTfm(Listing *dest, Instr *instr, Nat line) {
			icastTfm(dest, instr, line);
		}

		void RemoveInvalid::callFloatTfm(Listing *dest, Instr *instr, Nat line) {
			Size s = instr->src().size();
			Engine &e = engine();

			*dest << call(instr->src(), ValType(s, false));
			*dest << sub(ptrStack, ptrConst(s));
			*dest << fstp(xRel(s, ptrStack, Offset()));
			*dest << pop(instr->dest());
		}

		void RemoveInvalid::retFloatTfm(Listing *dest, Instr *instr, Nat line) {
			Size s = instr->src().size();
			Engine &e = engine();

			*dest << push(instr->src());
			*dest << fld(xRel(s, ptrStack, Offset()));
			*dest << add(ptrStack, ptrConst(s));
			*dest << ret(ValType(s, false));
		}

		void RemoveInvalid::fnParamTfm(Listing *dest, Instr *instr, Nat line) {
			params->push(Param(instr->src(), instr->dest()));
		}

		void RemoveInvalid::fnParamRefTfm(Listing *dest, Instr *instr, Nat line) {
			assert(false, L"Fixme when the interface has been fixed!");
		}

		static Operand offset(const Operand &src, Offset offset) {
			switch (src.type()) {
			case opVariable:
				return xRel(Size::sInt, src.variable(), offset);
			case opRegister:
				return xRel(Size::sInt, src.reg(), offset);
			default:
				assert(false, L"Can not generate offsets into this type!");
				return Operand();
			}
		}

		static void pushMemcpy(Listing *dest, const Operand &src) {
			Engine &e = dest->engine();

			if (src.size() <= Size::sInt) {
				*dest << push(src);
				return;
			}

			Nat size = roundUp(src.size().size32(), Nat(4));
			for (nat i = 0; i < size; i += 4) {
				*dest << push(offset(src, Offset(size - i)));
			}
		}

		void RemoveInvalid::fnCallTfm(Listing *dest, Instr *instr, Nat line) {
			// Idea: Scan backwards to find fnCall op-codes rather than saving them in an
			// array. This could catch stray fnParam op-codes if done right. We could also do it the
			// other way around, letting fnParam search for a terminating fnCall and be done there.
			Engine &e = engine();

			// Push all parameters we can right now.
			for (Nat i = params->count(); i > 0; i--) {
				Param &p = params->at(i - 1);

				if (p.copyFn.empty()) {
					// Memcpy using push...
					pushMemcpy(dest, p.src);
				} else {
					// Reserve stack space.
					Size s = p.src.size() + Size::sPtr.align();
					*dest << sub(ptrStack, ptrConst(s));
				}
			}

			// Now, we can clobber registers to our hearts content! At least until 'fnParamRef' is
			// implemented. 'fnParam' with two parameters requires one of them to be a variable,
			// while 'fnParamRef' is fine with keeping them anywhere.

			// Cumulated offset from esp.
			Offset paramOffset;

			for (Nat i = 0; i < params->count(); i++) {
				Param &p = params->at(i);

				Size s = p.src.size() + Size::sPtr.align();

				if (!p.copyFn.empty()) {
					// Copy it!
					*dest << lea(ptrA, p.src);
					*dest << push(ptrA);
					*dest << lea(ptrA, ptrRel(ptrStack, paramOffset));
					*dest << push(ptrA);
					*dest << call(p.copyFn, valVoid());
					*dest << add(ptrStack, ptrConst(Size::sPtr * 2));
				}

				paramOffset += s;
			}

			// Call the real function!
			Size rSize = instr->dest().size();
			*dest << call(instr->src(), ValType(rSize, false));

			// If this was a float, do some magic.
			if (instr->op() == op::fnCallFloat) {
				*dest << fstp(xRel(rSize, ptrStack, Offset()));
				*dest << pop(instr->dest());
			}

			// Pop the stack.
			if (paramOffset != Offset())
				*dest << add(ptrStack, ptrConst(paramOffset));

			// Clear parameters for next time.
			params->clear();
		}

		void RemoveInvalid::fnCallFloatTfm(Listing *dest, Instr *instr, Nat line) {
			fnCallTfm(dest, instr, line);
		}

	}
}
