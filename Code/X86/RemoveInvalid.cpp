#include "stdafx.h"
#include "RemoveInvalid.h"
#include "Listing.h"
#include "Exception.h"
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
			IMM_REG(bor),
			IMM_REG(band),
			IMM_REG(sub),
			IMM_REG(sbb),
			IMM_REG(bxor),
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

			TRANSFORM(fnParam),
			TRANSFORM(fnParamRef),
			TRANSFORM(fnCall),
			TRANSFORM(fnCallRef),
		};

		RemoveInvalid::Param::Param(Operand src, TypeDesc *type, Bool ref) : src(src), type(type), ref(ref) {}

		RemoveInvalid::RemoveInvalid() {}

		void RemoveInvalid::before(Listing *dest, Listing *src) {
			params = new (this) Array<Param>();

			used = usedRegs(dest->arena, src).used;

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

		Reg RemoveInvalid::unusedReg(Nat line) {
			return code::x86::unusedReg(used->at(line));
		}

		// ImmReg combination already supported?
		static bool supported(Instr *instr) {
			switch (instr->src().type()) {
			case opLabel:
			case opReference:
			case opConstant:
			case opObjReference:
			case opOffReference:
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
			assert(size.size32() <= Size::sInt.size32(), "The 64-bit transform should have fixed this!");

			Reg reg = unusedReg(line);
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

			Reg reg = unusedReg(line);
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
			assert(size.size32() <= Size::sInt.size32(), "Bytes not supported yet!");

			if (instr->dest().type() == opRegister) {
				*dest << instr;
				return;
			}

			// Only supported mmode is mul <reg>, <r/m>. Move dest into a register.
			Reg reg = unusedReg(line);
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

			// TODO: if 'src' is 'eax', then we're screwed.

			// Move dest into eax first.
			if (isByte)
				*to << bxor(eax, eax);
			*to << mov(asSize(eax, dest.size()), dest);

			if (srcConst) {
				if (used->has(ebx))
					*to << push(ebx);
				*to << mov(ebx, instr->src());
				newSrc = ebx;
			}

			// Note: we do not need to clear edx here, AsmOut will do that for us, ie. we treat edx
			// as an output-only register.
			*to << instr->alter(asSize(eax, dest.size()), newSrc);
			*to << mov(dest, asSize(eax, dest.size()));

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

			// TODO: if 'src' is 'eax', then we're screwed.

			// Move dest into eax first.
			if (isByte)
				*to << bxor(eax, eax);
			*to << mov(asSize(eax, dest.size()), dest);

			if (srcConst) {
				if (used->has(ebx))
					*to << push(ebx);
				*to << mov(ebx, instr->src());
				newSrc = ebx;
			}

			// Note: we do not need to clear edx here, AsmOut will do that for us, ie. we treat edx
			// as an output-only register.
			if (instr->op() == op::imod)
				*to << idiv(asSize(eax, dest.size()), newSrc);
			else
				*to << udiv(asSize(eax, dest.size()), newSrc);

			if (isByte) {
				*to << shr(eax, byteConst(8));
				*to << mov(dest, asSize(eax, dest.size()));
			} else {
				*to << mov(dest, edx);
			}


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
				Reg reg = asSize(unusedReg(line), size);

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
				Reg reg = asSize(unusedReg(line), Size::sInt);

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
			Operand to = instr->dest();
			Size sFrom = instr->src().size();
			Size sTo = to.size();

			if (instr->dest() == Operand(asSize(eax, sTo))) {
				*dest << instr;
				return;
			}

			bool toEax = to.type() == opRegister && same(to.reg(), eax);
			bool toEaxRel = to.type() == opRelative && same(to.reg(), eax);

			RegSet *used = this->used->at(line);
			bool saveEax = used->has(eax);
			bool saveEdx = used->has(edx);
			bool saveEcx = used->has(ecx);

			if (toEax)
				saveEax = false;
			if (sFrom != Size::sLong && sTo != Size::sLong)
				saveEdx = false;
			if (!toEaxRel)
				saveEcx = false;

			if (saveEdx)
				*dest << push(edx);
			if (saveEcx)
				*dest << push(ecx);
			if (saveEax)
				*dest << push(eax);

			if ((sFrom == Size::sByte && sTo == Size::sLong) ||
				(sFrom == Size::sLong && sTo == Size::sByte)) {
				*dest << instr->alterDest(eax);
				*dest << instr->alter(asSize(eax, sTo), eax);
			} else {
				*dest << instr->alterDest(asSize(eax, sTo));
			}

			if (!toEax) {
				if (toEaxRel) {
					// Read the old eax...
					*dest << mov(ptrC, ptrRel(ptrStack, Offset()));
					to = xRel(to.size(), ptrC, to.offset());
				}

				if (sTo == Size::sLong) {
					*dest << mov(low32(to), eax);
					*dest << mov(high32(to), edx);
				} else {
					*dest << mov(to, asSize(eax, sTo));
				}
			}

			if (saveEax)
				*dest << pop(eax);
			if (saveEcx)
				*dest << pop(ecx);
			if (saveEdx)
				*dest << pop(edx);
		}

		void RemoveInvalid::ucastTfm(Listing *dest, Instr *instr, Nat line) {
			icastTfm(dest, instr, line);
		}

		void RemoveInvalid::fnParamTfm(Listing *dest, Instr *instr, Nat line) {
			TypeInstr *ti = as<TypeInstr>(instr);
			if (!ti) {
				throw new (this) InvalidValue(S("Expected a TypeInstr for 'fnParam'."));
			}

			params->push(Param(ti->src(), ti->type, false));
		}

		void RemoveInvalid::fnParamRefTfm(Listing *dest, Instr *instr, Nat line) {
			TypeInstr *ti = as<TypeInstr>(instr);
			if (!ti) {
				throw new (this) InvalidValue(S("Expected a TypeInstr for 'fnParamRef'."));
			}

			params->push(Param(ti->src(), ti->type, true));
		}

		static Operand offset(const Operand &src, Offset offset) {
			switch (src.type()) {
			case opVariable:
				return xRel(Size::sInt, src.var(), src.offsetRef() + offset);
			case opRelative:
				return xRel(Size::sInt, src.reg(), src.offsetRef() + offset);
			default:
				assert(false, L"Can not generate offsets into this type!");
				return Operand();
			}
		}

		static void pushMemcpy(Listing *dest, const Operand &src) {
			if (src.size().size32() <= Size::sInt.size32()) {
				*dest << push(src);
				return;
			}

			Nat size = roundUp(src.size().size32(), Nat(4));
			for (nat i = 0; i < size; i += 4) {
				*dest << push(offset(src, Offset(size - i - 4)));
			}
		}

		static void inlinedMemcpy(Listing *to, const Operand &src, Offset offset, Size sz) {
			Nat size = roundUp(sz.size32(), Nat(4));
			// All registers used here are destroyed during function calls.
			if (src.type() != opRegister || !same(src.reg(), ptrA))
				*to << mov(ptrA, src);
			for (nat i = 0; i < size; i += 4) {
				*to << mov(edx, intRel(ptrA, Offset(i)));
				*to << mov(intRel(ptrStack, Offset(i) + offset), edx);
			}
		}

		// Subtract 'offset' from ptrStack if required. Resets it to 0 afterwards. Used to implement
		// 'lazy' subtraction of ptrStack in 'fnCall' below.
		static void subStack(Listing *dest, Offset &offset) {
			if (offset != Offset())
				*dest << sub(ptrStack, ptrConst(offset));
			offset = Offset();
		}

		void RemoveInvalid::fnCall(Listing *dest, TypeInstr *instr, Array<Param> *params) {
			assert(instr->src().type() != opRegister, L"Not supported.");

			// Returning a reference?
			Bool retRef = instr->op() == op::fnCallRef;

			// Do we need a parameter for the return value?
			if (resultParam(instr->type)) {
				Nat id = instr->member ? 1 : 0;
				// In case there are no parameters, the result parameter becomes the only one. This is not
				// really compliant with the calling convention, but there is nothing else we can do!
				id = min(id, params->count());
				params->insert(id, Param(instr->dest(), null, false));
			} else if (retRef) {
				// Perhaps we need to store the result on the stack?
				if (instr->dest().type() == opRegister)
					*dest << push(instr->dest());
			}

			// Push all parameters we can right now. For references and things that need a copy
			// constructor, store the address on the stack for now and get back to them later.
			Offset delayedSub;
			for (Nat i = params->count(); i > 0; i--) {
				Param &p = params->at(i - 1);
				bool first = i == 1;

				if (!p.type) {
					subStack(dest, delayedSub);

					if (retRef) {
						*dest << push(instr->dest());
					} else {
						// We need an additional register for this. Do it later!
						*dest << push(ptrConst(0));
					}
				} else if (as<ComplexDesc>(p.type) == null && !p.ref) {
					subStack(dest, delayedSub);

					// Push it to the stack now!
					pushMemcpy(dest, p.src);
				} else {
					// Copy the parameter later.
					Size s = p.type->size();
					s += Size::sPtr.alignment();

					// Note: Not needed for the first parameter. We do not have time to clobber
					// registers until we apply the first parameter, so we might as well just push
					// it straight away.
					if (!first && p.ref && p.src.hasRegister()) {
						// Include everything but the last 4 bytes in the 'sub' operation. We use a
						// 'push' for those since 'push src' is able to handle more addressing modes
						// compared to 'mov [ptrBase + 0x??], src'
						delayedSub += s;
						delayedSub -= Offset::sPtr;
						subStack(dest, delayedSub);

						// Store the source of the reference here for later. We might clobber this
						// register during the next phase!
						*dest << push(p.src);
					} else {
						// Remember that we shall adjust esp, so that multiple parameters can be
						// condensed into a single sub instruction.
						delayedSub += s;
						// *dest << sub(ptrStack, ptrConst(s));
					}
				}
			}

			subStack(dest, delayedSub);

			// Now, we can use any registers we like!
			// Note: If 'retRef' is false and we require a parameter for the return value, we know
			// that the return value reside in memory somewhere, otherwise we can not use 'lea' with it!

			// Cumulated offset from esp.
			Offset paramOffset;

			for (Nat i = 0; i < params->count(); i++) {
				Param &p = params->at(i);
				bool first = i == 0;

				Size s = p.type ? p.type->size() : Size::sPtr;
				s += Size::sPtr.alignment();

				if (!p.type) {
					if (!retRef) {
						*dest << lea(ptrA, p.src);
						*dest << mov(ptrRel(ptrStack, paramOffset), ptrA);
					}
				} else if (ComplexDesc *c = as<ComplexDesc>(p.type)) {
					if (!first && p.ref && p.src.hasRegister()) {
						// If the pointer was pushed on the stack, use that as we might have clobbered a used register.
						*dest << push(ptrRel(ptrStack, paramOffset));
					} else if (p.ref) {
						*dest << push(p.src);
					} else {
						*dest << lea(ptrA, p.src);
						*dest << push(ptrA);
					}

					*dest << lea(ptrA, ptrRel(ptrStack, paramOffset + Offset::sPtr));
					*dest << push(ptrA);
					*dest << call(c->ctor, Size());
					*dest << add(ptrStack, ptrConst(Size::sPtr * 2));
				} else if (p.ref) {
					// Copy it using an inlined memcpy.
					if (!first && p.src.hasRegister()) {
						*dest << mov(ptrA, ptrRel(ptrStack, paramOffset));
						inlinedMemcpy(dest, ptrA, paramOffset, p.type->size());
					} else {
						inlinedMemcpy(dest, p.src, paramOffset, p.type->size());
					}
				}

				paramOffset += s;
			}

			// Call the function! (We do not need to analyze register usage anymore, this is fine).
			*dest << call(instr->src(), Size());

			// Pop the stack.
			if (paramOffset != Offset())
				*dest << add(ptrStack, ptrConst(paramOffset));

			// Handle the return value if needed.
			if (PrimitiveDesc *p = as<PrimitiveDesc>(instr->type)) {
				Operand to = instr->dest();

				if (retRef) {
					if (to.type() == opRegister) {
						// Previously stored on the stack, restore it!
						*dest << pop(ptrC);
					} else {
						*dest << mov(ptrC, to);
					}
					to = xRel(p->size(), ptrC, Offset());
				}

				switch (p->v.kind()) {
				case primitive::none:
					break;
				case primitive::integer:
				case primitive::pointer:
					if (to.type() == opRegister && same(to.reg(), ptrA)) {
						// Nothing to do!
					} else if (to.size() == Size::sLong) {
						*dest << mov(high32(to), edx);
						*dest << mov(low32(to), eax);
					} else {
						*dest << mov(to, asSize(ptrA, to.size()));
					}
					break;
				case primitive::real:
					if (to.type() == opRegister) {
						*dest << sub(ptrStack, ptrConst(to.size()));
						*dest << fstp(xRel(to.size(), ptrStack, Offset()));
						if (to.size() == Size::sDouble) {
							*dest << pop(low32(to));
							*dest << pop(high32(to));
						} else {
							*dest << pop(to);
						}
					} else {
						*dest << fstp(to);
					}
					break;
				}
			}
		}

		void RemoveInvalid::fnCallTfm(Listing *dest, Instr *instr, Nat line) {
			// Idea: Scan backwards to find fnCall op-codes rather than saving them in an
			// array. This could catch stray fnParam op-codes if done right. We could also do it the
			// other way around, letting fnParam search for a terminating fnCall and be done there.

			TypeInstr *t = as<TypeInstr>(instr);
			if (!t) {
				throw new (this) InvalidValue(S("Expected a TypeInstr for 'fnCall'."));
			}

			fnCall(dest, t, params);

			params->clear();
		}

		void RemoveInvalid::fnCallRefTfm(Listing *dest, Instr *instr, Nat line) {
			// Idea: Scan backwards to find fnCall op-codes rather than saving them in an
			// array. This could catch stray fnParam op-codes if done right. We could also do it the
			// other way around, letting fnParam search for a terminating fnCall and be done there.

			TypeInstr *t = as<TypeInstr>(instr);
			if (!t) {
				throw new (this) InvalidValue(S("Expected a TypeInstr for 'fnCallRef'."));
			}

			fnCall(dest, t, params);

			params->clear();
		}

	}
}
