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

			TRANSFORM(fnRet),
			TRANSFORM(fnRetRef),
		};

		Layout::Active::Active(Part part, Label pos) : part(part), pos(pos) {}

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

			// Figure out which registers we need to spill.
			{
				toPreserve = new (this) RegSet();
				RegSet *notPreserved = fnDirtyRegs(engine());
				RegSet *used = allUsedRegs(src);

				for (RegSet::Iter i = used->begin(); i != used->end(); ++i) {
					if (!notPreserved->has(i.v()))
						toPreserve->put(i.v());
				}
			}

			// Compute the layout.
			Nat spilled = toPreserve->count();
			if (result->memory)
				spilled++; // The hidden parameter needs to be spilled!

			layout = code::x64::layout(src, params, spilled);

			// The EH table.
			activeParts = new (this) Array<Active>();
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
			*dest << alignAs(Size::sPtr);
			*dest << dest->meta();

			// Output metadata table.
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

			// Output the table containing active parts. Used by the exception handling mechanism.
			*dest << alignAs(Size::sPtr);
			// Table contents. Each 'row' is 8 bytes.
			for (Nat i = 0; i < activeParts->count(); i++) {
				const Active &a = activeParts->at(i);
				*dest << lblOffset(a.pos);
				*dest << dat(natConst(a.part.key()));
			}

			// Table size.
			*dest << dat(ptrConst(activeParts->count()));
			// Owner.
			*dest << dat(objPtr(owner));
		}

		Operand Layout::resolve(Listing *src, const Operand &op) {
			return resolve(src, op, op.size());
		}

		Operand Layout::resolve(Listing *src, const Operand &op, const Size &size) {
			if (op.type() != opVariable)
				return op;

			Var v = op.var();
			if (!src->accessible(v, part))
				throw VariableUseError(v, part);
			return xRel(size, ptrFrame, layout->at(v.key()) + op.offset());
		}

		void Layout::spillParams(Listing *dest) {
			Array<Var> *all = dest->allParams();

			for (Nat i = 0; i < params->registerCount(); i++) {
				Param info = params->registerAt(i);
				if (info.size() == 0)
					continue; // Not used.
				if (info.id() == Param::returnId) {
					*dest << mov(ptrRel(ptrFrame, resultParam()), params->registerSrc(i));
					continue;
				}

				Offset to = layout->at(all->at(info.id()).key());
				to += Offset(info.offset());

				Size size(info.size());
				Reg r = asSize(params->registerSrc(i), size);
				if (r == noReg) {
					// Size not natively supported. Round up!
					size += Size::sInt.alignment();
					r = asSize(params->registerSrc(i), size);
				}
				*dest << mov(xRel(size, ptrFrame, to), r);
			}
		}

		void Layout::prologTfm(Listing *dest, Listing *src, Nat line) {
			*dest << push(ptrFrame);
			*dest << mov(ptrFrame, ptrStack);

			// Notify that we've generated the prolog.
			*dest << prolog();

			// Allocate stack space.
			if (layout->last() != Offset())
				*dest << sub(ptrStack, ptrConst(layout->last()));

			// Keep track of offsets.
			Offset offset = -Offset::sPtr;

			// Save registers we need to preserve.
			for (RegSet::Iter i = toPreserve->begin(); i != toPreserve->end(); ++i) {
				*dest << mov(ptrRel(ptrFrame, offset), asSize(i.v(), Size::sPtr));
				*dest << preserve(ptrRel(ptrFrame, offset), asSize(i.v(), Size::sPtr));
				offset -= Offset::sPtr;
			}

			// Spill parameters to the stack.
			spillParams(dest);

			// Initialize the root block.
			initPart(dest, dest->root());
		}

		void Layout::epilogTfm(Listing *dest, Listing *src, Nat line) {
			// Destroy blocks. Note: we shall not modify 'part' nor alter the exception table as
			// this may be an early return from the function.
			Part oldPart = part;
			for (Part now = part; now != Part(); now = src->prev(now)) {
				destroyPart(dest, now, true, false);
			}
			part = oldPart;

			// Restore preserved registers.
			Offset offset = -Offset::sPtr;
			for (RegSet::Iter i = toPreserve->begin(); i != toPreserve->end(); ++i) {
				*dest << mov(asSize(i.v(), Size::sPtr), ptrRel(ptrFrame, offset));
				offset -= Offset::sPtr;
			}

			*dest << mov(ptrStack, ptrFrame);
			*dest << pop(ptrFrame);

			// Notify that we've generated the epilog.
			*dest << epilog();
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

				destroyPart(dest, now, false, true);
			}

			// Destroy the last one as well.
			destroyPart(dest, target, false, true);
		}

		Offset Layout::partId() {
			return -Offset::sPtr;
		}

		Offset Layout::resultParam() {
			Nat count = 1 + toPreserve->count();
			return -(Offset::sPtr * count);
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

			if (usingEH) {
				// Remember where the part started.
				Label lbl = dest->label();
				*dest << lbl;
				activeParts->push(Active(part, lbl));
			}
		}

		void Layout::destroyPart(Listing *dest, Part destroy, Bool preserveRax, Bool table) {
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
						saveResult(dest, dest->result);
						pushedRax = true;
					}

					if (when & freeIndirection) {
						if (when & freePtr) {
							*dest << mov(ptrDi, resolve(dest, v, Size::sPtr));
							*dest << call(dtor, Size());
						} else {
							*dest << mov(ptrDi, resolve(dest, v, Size::sPtr));
							*dest << mov(asSize(ptrDi, v.size()), xRel(v.size(), ptrDi, Offset()));
							*dest << call(dtor, Size());
						}
					} else {
						if (when & freePtr) {
							*dest << lea(ptrDi, resolve(dest, v));
							*dest << call(dtor, Size());
						} else {
							*dest << mov(asSize(ptrDi, v.size()), resolve(dest, v));
							*dest << call(dtor, Size());
						}
					}
					// TODO: Zero memory to avoid multiple destruction in rare cases?
				}
			}

			if (pushedRax) {
				restoreResult(dest, dest->result);
			}

			part = dest->prev(part);
			if (usingEH && table) {
				Label lbl = dest->label();
				*dest << lbl;
				activeParts->push(Active(part, lbl));
			}
		}

		static void returnLayout(Listing *dest, primitive::PrimitiveKind k, nat &i, nat &r, Offset offset) {
			static const Reg intReg[2] = { ptrA, ptrD };
			static const Reg realReg[2] = { xmm0, xmm1 };

			switch (k) {
			case primitive::none:
				break;
			case primitive::integer:
			case primitive::pointer:
				*dest << mov(intReg[i++], ptrRel(ptrSi, offset));
				break;
			case primitive::real:
				*dest << mov(realReg[r++], longRel(ptrSi, offset));
				break;
			}
		}

		// Memcpy using mov instructions.
		static void movMemcpy(Listing *to, Reg dest, Reg src, Size size) {
			Nat total = size.size64();
			Nat offset = 0;

			for (; offset + 8 <= total; offset += 8) {
				*to << mov(rdx, longRel(src, Offset(offset)));
				*to << mov(longRel(dest, Offset(offset)), rdx);
			}

			for (; offset + 4 <= total; offset += 4) {
				*to << mov(edx, intRel(src, Offset(offset)));
				*to << mov(intRel(dest, Offset(offset)), edx);
			}

			for (; offset + 1 <= total; offset += 1) {
				*to << mov(dl, byteRel(src, Offset(offset)));
				*to << mov(byteRel(dest, Offset(offset)), dl);
			}
		}

		// Put the return value into registers. Assumes ptrSi contains a pointer to the struct to be returned.
		static void returnSimple(Listing *dest, Result *result, SimpleDesc *type, Offset resultParam) {
			if (result->memory) {
				*dest << mov(ptrA, ptrRel(ptrFrame, resultParam));
				// Inline a memcpy using mov instructions.
				movMemcpy(dest, ptrA, ptrSi, type->size());
			} else {
				nat i = 0;
				nat r = 0;
				returnLayout(dest, result->part1, i, r, Offset());
				returnLayout(dest, result->part2, i, r, Offset::sPtr);
			}
		}

		static void returnPrimitive(Listing *dest, PrimitiveDesc *p, const Operand &value) {
			switch (p->v.kind()) {
			case primitive::none:
				break;
			case primitive::integer:
			case primitive::pointer:
				if (value.type() == opRegister && same(value.reg(), ptrA)) {
					// Already at the correct place!
				} else {
					// A simple 'mov' is enough!
					*dest << mov(asSize(ptrA, value.size()), value);
				}
				break;
			case primitive::real:
				// A simple 'mov' will do!
				*dest << mov(asSize(xmm0, value.size()), value);
				break;
			}
		}

		void Layout::fnRetTfm(Listing *dest, Listing *src, Nat line) {
			Operand value = resolve(src, src->at(line)->src());
			assert(value.size() == src->result->size(), L"Wrong size passed to fnRet!");

			// Handle the return value.
			if (PrimitiveDesc *p = as<PrimitiveDesc>(src->result)) {
				returnPrimitive(dest, p, value);
			} else if (ComplexDesc *c = as<ComplexDesc>(src->result)) {
				// Call the copy-ctor.
				*dest << lea(ptrSi, value);
				*dest << mov(ptrDi, ptrRel(ptrFrame, resultParam()));
				*dest << call(c->ctor, Size());
				// Set 'rax' to the address of the return value.
				*dest << mov(ptrA, ptrRel(ptrFrame, resultParam()));
			} else if (SimpleDesc *s = as<SimpleDesc>(src->result)) {
				*dest << lea(ptrSi, value);
				returnSimple(dest, result, s, resultParam());
			} else {
				assert(false);
			}

			epilogTfm(dest, src, line);
			*dest << ret(Size()); // We will not analyze registers anymore, Size() is fine.
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
				*dest << mov(ptrA, value);
				*dest << mov(asSize(xmm0, s), xRel(s, ptrA, Offset()));
				break;
			}
		}

		void Layout::fnRetRefTfm(Listing *dest, Listing *src, Nat line) {
			Operand value = resolve(src, src->at(line)->src());

			// Handle the return value.
			if (PrimitiveDesc *p = as<PrimitiveDesc>(src->result)) {
				returnPrimitiveRef(dest, p, value);
			} else if (ComplexDesc *c = as<ComplexDesc>(src->result)) {
				// Call the copy-ctor.
				*dest << mov(ptrSi, value);
				*dest << mov(ptrDi, ptrRel(ptrFrame, resultParam()));
				*dest << call(c->ctor, Size());
				// Set 'rax' to the address of the return value.
				*dest << mov(ptrA, ptrRel(ptrFrame, resultParam()));
			} else if (SimpleDesc *s = as<SimpleDesc>(src->result)) {
				*dest << mov(ptrSi, value);
				returnSimple(dest, result, s, resultParam());
			} else {
				assert(false);
			}

			epilogTfm(dest, src, line);
			*dest << ret(Size()); // We will not analyze registers anymore, Size() is fine.
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

			Size used;
			// Then, compute where to spill the remaining registers.
			for (Nat i = 0; i < all->count(); i++) {
				Var var = all->at(i);
				Offset &to = out->at(var.key());

				// Already computed -> no need to spill.
				if (to != Offset())
					continue;

				// Try to squeeze the preserved registers as tightly as possible while keeping alignment.
				// This is more concentrated than when pushing registers to the stack. On the stack, all
				// parameters are aligned to 8 bytes, while we do not need that here. We only need to keep
				// the natural alignment of the parameters to keep the CPU happy.
				Size sz = var.size();
				if (src->freeOpt(var) & freeIndirection)
					sz = Size::sPtr;

				// However, since we don't want to use multiple instructions to spill a single
				// parameter, we increase the size of some parameters a bit.
				if (asSize(ptrA, sz) == noReg)
					sz = sz + Size::sInt.alignment();

				used = (used + sz).alignedAs(sz);
				to = -(varOffset + used);
			}

			return used.aligned();
		}

		Array<Offset> *layout(Listing *src, Params *params, Nat spilled) {
			Array<Offset> *result = code::layout(src);

			// Saved registers and other things:
			Offset varOffset = Offset::sPtr * spilled;

			// Figure out which variables we need to spill into memory.
			varOffset += spillParams(result, src, params, varOffset);
			varOffset = varOffset.alignAs(Size::sPtr);

			// Update the layout of the other variables according to the size we needed for
			// parameters and spilled registers.
			Array<Var> *all = src->allVars();
			for (Nat i = 0; i < all->count(); i++) {
				Var var = all->at(i);
				Nat id = var.key();

				if (!src->isParam(var)) {
					Size s = var.size();
					if (src->freeOpt(var) & freeIndirection)
						s = Size::sPtr;
					result->at(id) = -(result->at(id) + s.aligned() + varOffset);
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
