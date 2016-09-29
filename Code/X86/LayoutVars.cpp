#include "stdafx.h"
#include "LayoutVars.h"
#include "Arena.h"
#include "Asm.h"
#include "Binary.h"
#include "Exception.h"
#include "SafeSeh.h"
#include "Seh.h"

namespace code {
	namespace x86 {

#define TRANSFORM(x) { op::x, &LayoutVars::x ## Tfm }

		const OpEntry<LayoutVars::TransformFn> LayoutVars::transformMap[] = {
			TRANSFORM(prolog),
			TRANSFORM(epilog),
			TRANSFORM(beginBlock),
			TRANSFORM(endBlock),
		};

		LayoutVars::LayoutVars(Binary *owner) : owner(owner) {}

		void LayoutVars::before(Listing *dest, Listing *src) {
			usingEH = src->exceptionHandler();
			RegSet *used = allUsedRegisters(src);
			add64(used);

			preserved = new (this) RegSet();
			RegSet *notPreserved = fnDirtyRegs(engine());

			// Any registers in 'all' which are not in 'notPreserved' needs to be properly stored
			// through this function call.
			for (RegSet::Iter i = used->begin(); i != used->end(); ++i) {
				if (!notPreserved->has(i.v()))
					preserved->put(i.v());
			}

			layout = code::x86::layout(src, preserved->count(), usingEH);

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

			Array<Variable> *vars = src->allVars();

			for (nat i = 0; i < vars->count(); i++) {
				Variable &v = vars->at(i);
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

			Variable v = src.variable();
			if (!listing->accessible(v, part))
				throw VariableUseError(v, part);
			return xRel(src.size(), ptrFrame, layout->at(v.key()) + src.offset());
		}

		// Zero the memory of a variable. 'initEax' should be true if we need to set eax to 0 before
		// using it as our zero. 'initEax' will be set to false, so that it is easy to use zeroVar
		// in a loop, causing only the first invocation to emit 'eax := 0'.
		static void zeroVar(Listing *dest, Offset start, Size size, bool &initEax) {
			Engine &e = dest->engine();

			nat s32 = size.size32();
			if (s32 == 0)
				return;

			if (initEax) {
				*dest << xor(eax, eax);
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

				Array<Variable> *vars = dest->allVars(b);
				// Go in reverse to make linear accesses in memory when we're using big variables.
				for (nat i = vars->count(); i > 0; i--) {
					Variable v = vars->at(i - 1);

					if (!dest->isParam(v))
						zeroVar(dest, layout->at(v.key()), v.size(), initEax);
				}

				if (usingEH)
					*dest << mov(engine(), intRel(ptrFrame, partId), natConst(part.key()));
			}
		}

		void LayoutVars::destroyPart(Listing *dest, Part destroy, bool preserveEax) {
			Engine &e = engine();

			if (destroy != part)
				throw BlockEndError();

			bool pushedEax = false;

			Array<Variable> *vars = dest->partVars(destroy);
			for (nat i = 0; i < vars->count(); i++) {
				Variable v = vars->at(i);

				Operand dtor = dest->freeFn(v);
				FreeOpt when = dest->freeOpt(v);

				if (!dtor.empty() && (when & freeOnBlockExit) == freeOnBlockExit) {
					if (preserveEax && !pushedEax) {
						*dest << push(ptrA);
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
				*dest << pop(ptrA);

			part = dest->prev(part);
			if (usingEH)
				*dest << mov(intRel(ptrFrame, partId), natConst(part.key()));
		}

		void LayoutVars::prologTfm(Listing *dest, Listing *src, Nat line) {
			Engine &e = engine();

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

				// Set ourselves the new frame.
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
			Engine &e = engine();

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

	}
}
