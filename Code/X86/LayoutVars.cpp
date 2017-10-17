#include "stdafx.h"
#include "LayoutVars.h"
#include "Arena.h"
#include "Asm.h"
#include "Binary.h"
#include "Exception.h"
#include "SafeSeh.h"
#include "Seh.h"
#include "../Layout.h"

namespace code {
	namespace x86 {

#define TRANSFORM(x) { op::x, &LayoutVars::x ## Tfm }

		const OpEntry<LayoutVars::TransformFn> LayoutVars::transformMap[] = {
			TRANSFORM(prolog),
			TRANSFORM(epilog),
			TRANSFORM(beginBlock),
			TRANSFORM(endBlock),

			TRANSFORM(fnRet),
			TRANSFORM(fnRetRef),
		};

		LayoutVars::LayoutVars(Binary *owner) : owner(owner) {}

		Operand LayoutVars::resultLoc() {
			if (memberFn) {
				return ptrRel(ptrFrame, Offset::sPtr * 3);
			} else {
				return ptrRel(ptrFrame, Offset::sPtr * 2);
			}
		}

		void LayoutVars::before(Listing *dest, Listing *src) {
			usingEH = src->exceptionHandler();
			resultParam = code::x86::resultParam(src->result);
			memberFn = src->member;

			RegSet *used = allUsedRegs(src);
			add64(used);

			preserved = new (this) RegSet();
			RegSet *notPreserved = fnDirtyRegs(engine());

			// Any registers in 'all' which are not in 'notPreserved' needs to be properly stored
			// through this function call.
			for (RegSet::Iter i = used->begin(); i != used->end(); ++i) {
				if (!notPreserved->has(i.v()))
					preserved->put(i.v());
			}

			layout = code::x86::layout(src, preserved->count(), usingEH, resultParam, memberFn);

			if (usingEH)
				binaryLbl = dest->label();
		}

		void LayoutVars::during(Listing *dest, Listing *src, Nat line) {
			static OpTable<TransformFn> t(transformMap, ARRAY_COUNT(transformMap));

			Instr *i = src->at(line);
			TransformFn f = t[i->op()];
			if (f) {
				(this->*f)(dest, src, line);
			} else {
				*dest << i->alter(resolve(src, i->dest()), resolve(src, i->src()));
			}
		}

		void LayoutVars::after(Listing *dest, Listing *src) {
			// NOTE: This table may not be aligned properly. On X86, this is not a problem, but it
			// might be on other platforms!

			if (usingEH) {
				*dest << binaryLbl;
				*dest << dat(objPtr(owner));
			}

			*dest << dest->meta();

			Array<Var> *vars = src->allVars();

			for (nat i = 0; i < vars->count(); i++) {
				Var &v = vars->at(i);
				Operand fn = src->freeFn(v);
				if (fn.empty())
					*dest << dat(ptrConst(Offset(0)));
				else
					*dest << dat(src->freeFn(v));
				*dest << dat(ptrConst(layout->at(v.key())));
			}
		}

		Operand LayoutVars::resolve(Listing *listing, const Operand &src) {
			if (src.type() != opVariable)
				return src;

			Var v = src.var();
			if (!listing->accessible(v, part))
				throw VariableUseError(v, part);
			return xRel(src.size(), ptrFrame, layout->at(v.key()) + src.offset());
		}

		// Zero the memory of a variable. 'initEax' should be true if we need to set eax to 0 before
		// using it as our zero. 'initEax' will be set to false, so that it is easy to use zeroVar
		// in a loop, causing only the first invocation to emit 'eax := 0'.
		static void zeroVar(Listing *dest, Offset start, Size size, bool &initEax) {
			nat s32 = size.size32();
			if (s32 == 0)
				return;

			if (initEax) {
				*dest << bxor(eax, eax);
				initEax = false;
			}

			for (nat i = 0; i < s32; i += 4) {
				if (s32 - i > 1) {
					*dest << mov(intRel(ptrFrame, start + Offset(i)), eax);
				} else {
					*dest << mov(byteRel(ptrFrame, start + Offset(i)), al);
				}
			}
		}

		void LayoutVars::initPart(Listing *dest, Part init) {
			if (part != dest->prev(init)) {
				throw BlockBeginError(L"Can not begin " + ::toS(init) + L" unless the current is "
									+ ::toS(dest->prev(init)) + L". Current is " + ::toS(part));
			}

			part = init;

			Block b = dest->first(part);
			if (Part(b) == part) {
				bool initEax = true;

				Array<Var> *vars = dest->allVars(b);
				// Go in reverse to make linear accesses in memory when we're using big variables.
				for (nat i = vars->count(); i > 0; i--) {
					Var v = vars->at(i - 1);

					if (!dest->isParam(v))
						zeroVar(dest, layout->at(v.key()), v.size(), initEax);
				}
			}

			if (usingEH)
				*dest << mov(intRel(ptrFrame, partId), natConst(part.key()));
		}

		static void saveResult(Listing *dest) {
			if (PrimitiveDesc *p = as<PrimitiveDesc>(dest->result)) {
				Size s = p->v.size();
				switch (p->v.kind()) {
				case primitive::none:
					break;
				case primitive::integer:
				case primitive::pointer:
					if (s == Size::sLong)
						*dest << push(ptrD);
					*dest << push(ptrA);
					break;
				case primitive::real:
					*dest << sub(ptrStack, ptrConst(s));
					*dest << fstp(xRel(s, ptrStack, Offset()));
					break;
				}
			} else {
				// In both cases we need to the address to the value on the stack.
				*dest << push(ptrA);
			}
		}

		static void restoreResult(Listing *dest) {
			if (PrimitiveDesc *p = as<PrimitiveDesc>(dest->result)) {
				Size s = p->v.size();
				switch (p->v.kind()) {
				case primitive::none:
					break;
				case primitive::integer:
				case primitive::pointer:
					*dest << pop(ptrA);
					if (s == Size::sLong)
						*dest << pop(ptrD);
					break;
				case primitive::real:
					*dest << fld(xRel(s, ptrStack, Offset()));
					*dest << add(ptrStack, ptrConst(s));
					break;
				}
			} else {
				// In both cases we need to the address to the value on the stack.
				*dest << pop(ptrA);
			}
		}

		void LayoutVars::destroyPart(Listing *dest, Part destroy, bool preserveEax) {
			if (destroy != part)
				throw BlockEndError();

			bool pushedEax = false;

			Array<Var> *vars = dest->partVars(destroy);
			for (nat i = 0; i < vars->count(); i++) {
				Var v = vars->at(i);

				Operand dtor = dest->freeFn(v);
				FreeOpt when = dest->freeOpt(v);

				if (!dtor.empty() && (when & freeOnBlockExit) == freeOnBlockExit) {
					if (preserveEax && !pushedEax) {
						saveResult(dest);
						pushedEax = true;
					}

					if (when & freePtr) {
						*dest << lea(ptrA, resolve(dest, v));
						*dest << push(ptrA);
						*dest << call(dtor, valVoid());
						*dest << add(ptrStack, ptrConst(Offset::sPtr));
					} else if (v.size() <= Size::sInt) {
						*dest << push(resolve(dest, v));
						*dest << call(dtor, valVoid());
						*dest << add(ptrStack, ptrConst(v.size()));
					} else {
						*dest << push(high32(resolve(dest, v)));
						*dest << push(low32(resolve(dest, v)));
						*dest << call(dtor, valVoid());
						*dest << add(ptrStack, ptrConst(v.size()));
					}

					// TODO? Zero memory to avoid multiple destruction in rare cases?
				}
			}

			if (pushedEax)
				restoreResult(dest);

			part = dest->prev(part);
			if (usingEH)
				*dest << mov(intRel(ptrFrame, partId), natConst(part.key()));
		}

		void LayoutVars::prologTfm(Listing *dest, Listing *src, Nat line) {
			// Set up stack frame.
			*dest << push(ptrFrame);
			*dest << mov(ptrFrame, ptrStack);

			// Allocate stack space.
			*dest << sub(ptrStack, ptrConst(layout->last()));

			// Keep track of offsets...
			Offset offset = -Offset::sPtr;

			// Extra data needed for exception handling.
			if (usingEH) {
				// Current part id.
				*dest << mov(intRel(ptrFrame, offset), natConst(0));
				partId = offset;
				offset -= Offset::sInt;

				// Owner.
				*dest << mov(ptrRel(ptrFrame, offset), binaryLbl);
				offset -= Offset::sPtr;

				// Standard SEH frame.
				*dest << mov(ptrRel(ptrFrame, offset), xConst(Size::sPtr, Word(&::x86SafeSEH)));
				offset -= Offset::sPtr;

				// Previous SEH frame
				*dest << threadLocal() << mov(ptrA, ptrRel(noReg, Offset()));
				*dest << mov(ptrRel(ptrFrame, offset), ptrA);

				// Set ourselves as the current frame.
				*dest << lea(ptrA, ptrRel(ptrFrame, offset));
				*dest << threadLocal() << mov(ptrRel(noReg, Offset()), ptrA);

				offset -= Offset::sPtr;
			}

			// Save any registers we need to preserve.
			for (RegSet::Iter i = preserved->begin(); i != preserved->end(); ++i) {
				*dest << mov(ptrRel(ptrFrame, offset), asSize(*i, Size::sPtr));
				offset -= Offset::sPtr;
			}

			// Initialize the root block.
			initPart(dest, dest->root());
		}

		void LayoutVars::epilogTfm(Listing *dest, Listing *src, Nat line) {
			// Destroy blocks. Note: we shall not modify 'part' as this may be an early return from
			// the function.
			Part oldPart = part;
			for (Part now = part; now != Part(); now = src->prev(now)) {
				destroyPart(dest, now, true);
			}
			part = oldPart;

			// Restore preserved registers.
			{
				Offset offset = -Offset::sPtr;
				if (usingEH)
					offset -= Offset::sPtr * 4;
				for (RegSet::Iter i = preserved->begin(); i != preserved->end(); ++i) {
					*dest << mov(asSize(*i, Size::sPtr), ptrRel(ptrFrame, offset));
					offset -= Offset::sPtr;
				}
			}

			if (usingEH) {
				// Remove the SEH. Note: ptrC is not preserved across function calls, so it is OK to use it here!
				// We can not use ptrA nor ptrD as rax == eax:edx
				*dest << mov(ptrC, ptrRel(ptrFrame, -Offset::sPtr * 4));
				*dest << threadLocal() << mov(ptrRel(noReg, Offset()), ptrC);
			}

			*dest << mov(ptrStack, ptrFrame);
			*dest << pop(ptrFrame);
		}

		void LayoutVars::beginBlockTfm(Listing *dest, Listing *src, Nat line) {
			initPart(dest, src->at(line)->src().part());
		}

		void LayoutVars::endBlockTfm(Listing *dest, Listing *src, Nat line) {
			Part target = src->at(line)->src().part();
			Part start = part;

			for (Part now = part; now != target; now = src->prev(now)) {
				if (now == Part())
					throw BlockEndError(L"Block " + ::toS(target) + L" is not a parent of " + ::toS(start));

				destroyPart(dest, now, false);
			}

			// Destroy the last one as well.
			destroyPart(dest, target, false);
		}

		static void returnPrimitive(Listing *dest, PrimitiveDesc *p, const Operand &value) {
			switch (p->v.kind()) {
			case primitive::none:
				break;
			case primitive::integer:
			case primitive::pointer:
				if (value.type() == opRegister && same(value.reg(), ptrA)) {
					// Already at the proper place!
				} else {
					// A simple 'mov' is enough!
					*dest << mov(asSize(ptrA, value.size()), value);
				}
				break;
			case primitive::real:
				// We need to load it on the FP stack.
				*dest << push(value);
				*dest << fld(xRel(value.size(), ptrStack, Offset()));
				*dest << add(ptrStack, ptrConst(value.size()));
				break;
			}
		}

		void LayoutVars::fnRetTfm(Listing *dest, Listing *src, Nat line) {
			Operand value = resolve(src, src->at(line)->src());

			if (PrimitiveDesc *p = as<PrimitiveDesc>(src->result)) {
				returnPrimitive(dest, p, value);
			} else if (ComplexDesc *c = as<ComplexDesc>(src->result)) {
				// Call the copy-constructor.
				*dest << lea(ptrA, value);
				*dest << push(ptrA);
				*dest << push(resultLoc());
				*dest << call(c->ctor, valVoid());
				*dest << add(ptrStack, ptrConst(Size::sPtr * 2));
				*dest << lea(ptrA, value);
			} else if (SimpleDesc *s = as<SimpleDesc>(src->result)) {
			} else {
				assert(false);
			}

			epilogTfm(dest, src, line);
			*dest << ret(valVoid());
		}

		static void returnPrimitiveRef(Listing *dest, PrimitiveDesc *p, const Operand &value) {
			Size s(p->v.size());
			switch (p->v.kind()) {
			case primitive::none:
				break;
			case primitive::integer:
			case primitive::pointer:
				// Always two 'mov'.
				*dest << mov(ptrA, value);
				*dest << mov(asSize(ptrA, s), xRel(s, ptrA, Offset()));
				break;
			case primitive::real:
				// Load to the FP stack.
				*dest << fld(xRel(s, ptrStack, Offset()));
				break;
			}
		}

		void LayoutVars::fnRetRefTfm(Listing *dest, Listing *src, Nat line) {
			Operand value = resolve(src, src->at(line)->src());

			if (PrimitiveDesc *p = as<PrimitiveDesc>(src->result)) {
				returnPrimitiveRef(dest, p, value);
			} else if (ComplexDesc *c = as<ComplexDesc>(src->result)) {
				// Call the copy-constructor.
				*dest << push(value);
				*dest << push(resultLoc());
				*dest << call(c->ctor, valVoid());
				*dest << add(ptrStack, ptrConst(Size::sPtr));
				*dest << pop(ptrA);
			} else if (SimpleDesc *s = as<SimpleDesc>(src->result)) {
			} else {
				assert(false);
			}

			epilogTfm(dest, src, line);
			*dest << ret(valVoid());
		}


		static void layoutParams(Array<Offset> *result, Listing *src, Bool resultParam, Bool member) {
			Offset offset = Offset::sPtr * 2; // old ebp and return address
			Array<Var> *params = src->allParams();
			for (Nat i = 0; i < params->count(); i++) {
				if (resultParam) {
					// Add space for the result parameter
					if (i == 0 && !member)
						offset += Size::sPtr;
					else if (i == 1 && member)
						offset += Size::sPtr;
				}

				Var var = params->at(i);
				Nat id = var.key();
				result->at(id) = offset;
				offset = (offset + var.size().aligned()).alignAs(Size::sPtr);
			}
		}

		Array<Offset> *layout(Listing *src, Nat savedRegs, Bool usingEH, Bool resultParam, Bool member) {
			Array<Offset> *result = code::layout(src);
			Array<Var> *all = src->allVars();

			Offset varOffset;
			// Exception handler frame.
			if (usingEH)
				varOffset += Size::sPtr * 4;
			// Saved registers.
			varOffset += Size::sPtr * savedRegs;

			// Update all variables.
			for (nat i = 0; i < all->count(); i++) {
				Var var = all->at(i);
				Nat id = var.key();

				if (src->isParam(var)) {
					// Handled later.
				} else {
					result->at(id) = -(result->at(id) + var.size().aligned() + varOffset);
				}
			}

			// Update all parameters.
			layoutParams(result, src, resultParam, member);

			result->last() = result->last() + varOffset;
			return result;
		}

	}
}
