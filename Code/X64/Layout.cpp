#include "stdafx.h"
#include "Layout.h"
#include "Exception.h"
#include "Asm.h"
#include "../Binary.h"
#include "../Layout.h"

namespace code {
	namespace x64 {

#define TRANSFORM(x) { op::x, &Layout::x ## Tfm }

		const OpEntry<Layout::TransformFn> Layout::transformMap[] = {
			TRANSFORM(prolog),
			TRANSFORM(epilog),
			TRANSFORM(beginBlock),
			TRANSFORM(endBlock),
		};

		Layout::Layout(Binary *owner) : owner(owner) {}

		void Layout::before(Listing *dest, Listing *src) {
			// Initialize some state.
			part = Part();
			usingEH = src->exceptionHandler();

			// Compute where the result is to be stored.
			result = code::x64::result(src->result);

			// Compute the layout of all parameters.
			params = new (this) Params();

			// Hidden return parameter?
			if (result->memory)
				params->add(Param::returnId, ptrPrimitive());

			// Add the rest of them.
			Array<Var> *p = src->allParams();
			for (Nat i = 0; i < p->count(); i++) {
				params->add(i, src->paramDesc(p->at(i)));
			}

			// Compute the layout. TODO: Figure out how many registers we need to spill!
			layout = code::x64::layout(src, params, 0, usingEH);
		}

		void Layout::during(Listing *dest, Listing *src, Nat line) {
			static OpTable<TransformFn> t(transformMap, ARRAY_COUNT(transformMap));

			Instr *i = src->at(line);
			TransformFn f = t[i->op()];
			if (f) {
				(this->*f)(dest, src, line);
			} else {
				*dest << i->alter(resolve(src, i->dest()), resolve(src, i->src()));
			}
		}

		void Layout::after(Listing *dest, Listing *src) {
			*dest << dest->meta();
			// TODO: Output metadata table.
		}

		Operand Layout::resolve(Listing *src, const Operand &op) {
			if (op.type() != opVariable)
				return op;

			Var v = op.var();
			if (!src->accessible(v, part))
				throw VariableUseError(v, part);
			return xRel(op.size(), ptrFrame, layout->at(v.key()) + op.offset());
		}

		void Layout::prologTfm(Listing *dest, Listing *src, Nat line) {
			*dest << push(ptrFrame);
			*dest << mov(ptrFrame, ptrStack);

			// Allocate stack space.
			*dest << sub(ptrStack, ptrConst(layout->last()));

			// Keep track of offsets.
			Offset offset = -Offset::sPtr;

			// Output the exception handler frame.
			if (usingEH) {
				// Current part id.
				*dest << mov(intRel(ptrFrame, offset), natConst(0));
				offset -= Offset::sPtr;

				// Owner (needs two instructions, since both will hit memory).
				*dest << mov(ptrA, objPtr(owner));
				*dest << mov(ptrRel(ptrFrame, offset), ptrA);
				offset -= Offset::sPtr;
			}

			// TODO: Save registers we need to preserve!

			// Initialize the root block.
			initPart(dest, dest->root());
		}

		void Layout::epilogTfm(Listing *dest, Listing *src, Nat line) {
			// Destroy blocks. Note: we shall not modify 'part' as this may be an early return from the function.
			Part oldPart = part;
			for (Part now = part; now != Part(); now = src->prev(now)) {
				destroyPart(dest, now, true);
			}
			part = oldPart;

			// TODO: Restore preserved registers!

			*dest << mov(ptrStack, ptrFrame);
			*dest << pop(ptrFrame);
		}

		void Layout::beginBlockTfm(Listing *dest, Listing *src, Nat line) {
			initPart(dest, src->at(line)->src().part());
		}

		void Layout::endBlockTfm(Listing *dest, Listing *src, Nat line) {
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

		Offset Layout::partId() {
			return -Offset::sPtr;
		}

		// Zero the memory of a variable. 'initEax' should be true if we need to set eax to 0 before
		// using it as our zero. 'initRax' will be set to false, so that it is easy to use zeroVar
		// in a loop, causing only the first invocation to emit 'rax := 0'.
		static void zeroVar(Listing *dest, Offset start, Size size, Bool &initRax) {
			nat s64 = size.size64();
			if (s64 == 0)
				return;

			if (initRax) {
				*dest << bxor(rax, rax);
				initRax = false;
			}

			for (nat i = 0; i < s64; i += 8) {
				if (s64 - i > 4) {
					*dest << mov(longRel(ptrFrame, start + Offset(i)), rax);
				} else if (s64 - i > 1) {
					*dest << mov(intRel(ptrFrame, start + Offset(i)), eax);
				} else {
					*dest << mov(byteRel(ptrFrame, start + Offset(i)), al);
				}
			}
		}

		void Layout::initPart(Listing *dest, Part init) {
			if (part != dest->prev(init)) {
				throw BlockBeginError(L"Can not begin " + ::toS(init) + L" unless the current is "
									+ ::toS(dest->prev(init)) + L". Current is " + ::toS(part));
			}

			part = init;

			Block b = dest->first(part);
			if (Part(b) == part) {
				Bool initEax = true;
				Array<Var> *vars = dest->allVars(b);
				// Go in reverse to make linear accesses in memory when we're using big variables.
				for (Nat i = vars->count(); i > 0; i--) {
					Var v = vars->at(i - 1);

					if (!dest->isParam(v))
						zeroVar(dest, layout->at(v.key()), v.size(), initEax);
				}
			}

			if (usingEH)
				*dest << mov(intRel(ptrFrame, partId()), natConst(part.key()));
		}

		void Layout::destroyPart(Listing *dest, Part destroy, Bool preserveRax) {
			if (destroy != part)
				throw BlockEndError();

			Bool pushedRax = false;
			Array<Var> *vars = dest->partVars(destroy);
			for (Nat i = 0; i < vars->count(); i++) {
				Var v = vars->at(i);

				Operand dtor = dest->freeFn(v);
				FreeOpt when = dest->freeOpt(v);

				if (!dtor.empty() && (when & freeOnBlockExit) == freeOnBlockExit) {
					if (preserveRax && !pushedRax) {
						// We need to keep the stack 16-byte aligned.
						*dest << push(ptrA);
						*dest << push(ptrA);
						pushedRax = true;
					}

					if (when & freePtr) {
						*dest << lea(ptrDi, resolve(dest, v));
						*dest << call(dtor, valVoid());
					} else {
						*dest << mov(asSize(ptrDi, v.size()), resolve(dest, v));
						*dest << call(dtor, valVoid());
					}

					// TODO: Zero memory to avoid multiple destruction in rare cases?
				}
			}

			if (pushedRax) {
				*dest << pop(ptrA);
				*dest << pop(ptrA);
			}

			part = dest->prev(part);
			if (usingEH)
				*dest << mov(intRel(ptrFrame, partId()), natConst(part.key()));
		}



		static Size spillParams(Array<Offset> *out, Listing *src, Params *params, Offset varOffset) {
			// NOTE: We could avoid spilling primitives to memory, as we do not generally use those
			// registers from code generated for the generic platform. However, we would need to
			// save as soon as we perform a function call anyway, which is probably more expensive
			// than just spilling them unconditionally in the function prolog.

			Array<Var> *all = src->allParams();

			{
				// Start by computing the locations of parameters passed on the stack.
				Offset stackOffset = Offset::sPtr * 2;
				for (Nat i = 0; i < params->stackCount(); i++) {
					Nat paramId = params->stackAt(i); // Should never be 'Param::returnId'
					Var var = all->at(paramId);

					out->at(var.key()) = stackOffset;
					stackOffset = (stackOffset + var.size().aligned()).alignAs(Size::sPtr);
				}
			}

			// Then, compute where to spill the remaining registers.
			Size used;
			for (Nat i = 0; i < all->count(); i++) {
				Var var = all->at(i);
				Offset &to = out->at(var.key());

				// Already computed -> no need to spill.
				if (to != Offset())
					continue;

				used += var.size();
				to = -(varOffset + used.aligned());
			}

			return used.aligned();
		}

		Array<Offset> *layout(Listing *src, Params *params, Nat spilled, Bool usingEH) {
			Array<Offset> *result = code::layout(src);

			// Saved registers:
			Offset varOffset = Offset::sPtr * spilled;

			// Exception handling uses an additional 2 pointers on the stack.
			if (usingEH)
				varOffset += Offset::sPtr * 2;

			// Figure out which variables we need to spill into memory.
			varOffset += spillParams(result, src, params, varOffset);

			// Update the layout of the other variables according to the size we needed for
			// parameters and spilled registers.
			Array<Var> *all = src->allVars();
			for (Nat i = 0; i < all->count(); i++) {
				Var var = all->at(i);
				Nat id = var.key();

				if (!src->isParam(var)) {
					result->at(id) = -(result->at(id) + var.size().aligned() + varOffset);
				}
			}

			result->last() += varOffset;
			if (result->last().v64() & 0xF) {
				// We need to be aligned to 64 bits. Otherwise the SIMD-operations used widely on
				// X86-64 will not work properly.
				result->last() += Offset::sPtr;
			}

			return result;
		}

	}
}
