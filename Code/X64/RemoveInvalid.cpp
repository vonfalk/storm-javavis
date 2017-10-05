#include "stdafx.h"
#include "RemoveInvalid.h"
#include "Asm.h"
#include "Params.h"
#include "../Listing.h"
#include "../UsedRegs.h"
#include "../Exception.h"
#include "Utils/Bitwise.h"

namespace code {
	namespace x64 {

		ParamInfo::ParamInfo(TypeDesc *desc, const Operand &src, Bool ref)
			: type(desc), src(src), ref(ref), lea(false) {}

		ParamInfo::ParamInfo(TypeDesc *desc, const Operand &src, Bool ref, Bool lea)
			: type(desc), src(src), ref(ref), lea(lea) {}


#define TRANSFORM(x) { op::x, &RemoveInvalid::x ## Tfm }
#define IMM_REG(x) { op::x, &RemoveInvalid::immRegTfm }
#define DEST_W_REG(x) { op::x, &RemoveInvalid::destRegWTfm }
#define DEST_RW_REG(x) { op::x, &RemoveInvalid::destRegRwTfm }

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

			DEST_W_REG(lea),
			DEST_W_REG(icast),
			DEST_W_REG(ucast),
			DEST_RW_REG(mul),

			TRANSFORM(beginBlock),
			TRANSFORM(endBlock),

			TRANSFORM(fnCall),
			TRANSFORM(fnCallRef),
			TRANSFORM(fnParam),
			TRANSFORM(fnParamRef),
			// fnRet and fnRetRef are handled in the Layout transform.

			TRANSFORM(idiv),
			TRANSFORM(udiv),
			TRANSFORM(imod),
			TRANSFORM(umod),
		};

		RemoveInvalid::RemoveInvalid() {}

		void RemoveInvalid::before(Listing *dest, Listing *src) {
			used = usedRegs(dest->arena, src).used;
			large = new (this) Array<Operand>();
			lblLarge = dest->label();
			params = new (this) Array<ParamInfo>();
		}

		void RemoveInvalid::during(Listing *dest, Listing *src, Nat line) {
			static OpTable<TransformFn> t(transformMap, ARRAY_COUNT(transformMap));

			Instr *i = src->at(line);
			switch (i->op()) {
			case op::call:
			case op::fnCall:
			case op::fnCallRef:
			case op::jmp:
				// Nothing needed. We deal with these later on in the chain.
				break;
			default:
				i = extractNumbers(i);
				i = extractComplex(dest, i, line);
				break;
			}

			TransformFn f = t[i->op()];
			if (f) {
				(this->*f)(dest, i, line);
			} else {
				*dest << i;
			}
		}

		void RemoveInvalid::after(Listing *dest, Listing *src) {
			// Output all constants.
			*dest << lblLarge;
			for (Nat i = 0; i < large->count(); i++) {
				*dest << dat(large->at(i));
			}
		}

		Instr *RemoveInvalid::extractNumbers(Instr *i) {
			Operand src = i->src();
			if (src.type() == opConstant && src.size() == Size::sWord && !singleInt(src.constant())) {
				i = i->alterSrc(longRel(lblLarge, Offset::sWord*large->count()));
				large->push(src);
			}

			// Labels are also constants.
			if (src.type() == opLabel) {
				i = i->alterSrc(ptrRel(lblLarge, Offset::sWord*large->count()));
				large->push(src);
			}

			// Since writing to a constant is not allowed, we will not attempt to extract 'dest'.
			return i;
		}

		static bool isComplex(Listing *l, Operand op) {
			if (op.type() != opVariable)
				return false;

			Var v = op.var();
			TypeDesc *t = l->paramDesc(v);
			if (!t)
				return false;

			return as<ComplexDesc>(t) != null;
		}

		Instr *RemoveInvalid::extractComplex(Listing *to, Instr *i, Nat line) {
			// Complex parameters are passed as a pointer. Dereference these by inserting a 'mov' instruction.
			RegSet *regs = used->at(line);
			if (isComplex(to, i->src())) {
				Reg reg = asSize(unusedReg(regs), Size::sPtr);
				regs = new (this) RegSet(*regs);
				regs->put(reg);

				*to << mov(reg, i->src());
				i = i->alterSrc(ptrRel(reg, Offset()));
			}

			if (isComplex(to, i->dest())) {
				Reg reg = asSize(unusedReg(regs), Size::sPtr);
				*to << mov(reg, i->dest());
				i = i->alterDest(ptrRel(reg, Offset()));
			}

			return i;
		}

		// Is this src+dest combination supported for immReg op-codes?
		static bool supported(Instr *instr) {
			// Basically: one operand has to be a register, except when 'src' is a constant.
			switch (instr->src().type()) {
			case opConstant:
				// If a constant remains this far, it is small enough to be an immediate value!
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
			Reg reg = unusedReg(used->at(line));
			reg = asSize(reg, size);
			*dest << mov(reg, instr->src());
			*dest << instr->alterSrc(reg);
		}

		void RemoveInvalid::destRegWTfm(Listing *dest, Instr *instr, Nat line) {
			if (instr->dest().type() == opRegister) {
				*dest << instr;
				return;
			}

			Reg reg = unusedReg(used->at(line));
			reg = asSize(reg, instr->dest().size());
			*dest << instr->alterDest(reg);
			*dest << mov(instr->dest(), reg);
		}

		void RemoveInvalid::destRegRwTfm(Listing *dest, Instr *instr, Nat line) {
			if (instr->dest().type() == opRegister) {
				*dest << instr;
				return;
			}

			Reg reg = unusedReg(used->at(line));
			reg = asSize(reg, instr->dest().size());
			*dest << mov(reg, instr->dest());
			*dest << instr->alterDest(reg);
			*dest << mov(instr->dest(), reg);
		}

		void RemoveInvalid::beginBlockTfm(Listing *dest, Instr *instr, Nat line) {
			currentPart = instr->src().part();
		}

		void RemoveInvalid::endBlockTfm(Listing *dest, Instr *instr, Nat line) {
			Part ended = instr->src().part();
			currentPart = dest->parent(ended);
		}

		void RemoveInvalid::fnParamTfm(Listing *dest, Instr *instr, Nat line) {
			TypeInstr *i = as<TypeInstr>(instr);
			if (!i) {
				TODO(L"REMOVE ME"); return;
				throw InvalidValue(L"Expected a TypeInstr for 'fnParam'.");
			}

			params->push(ParamInfo(i->type, i->src(), false));
		}

		void RemoveInvalid::fnParamRefTfm(Listing *dest, Instr *instr, Nat line) {
			TypeInstr *i = as<TypeInstr>(instr);
			if (!i) {
				TODO(L"REMOVE ME"); return;
				throw InvalidValue(L"Expected a TypeInstr for 'fnParamRef'.");
			}

			params->push(ParamInfo(i->type, i->src(), true));
		}

		static bool hasComplex(Array<ParamInfo> *params) {
			for (Nat i = 0; i < params->count(); i++)
				if (as<ComplexDesc>(params->at(i).type))
					return true;
			return false;
		}

		static Params *paramLayout(Array<ParamInfo> *params) {
			Params *r = new (params) Params();
			for (Nat i = 0; i < params->count(); i++)
				r->add(i, params->at(i).type);
			return r;
		}

		static Nat stackParamsSize(Array<ParamInfo> *src, Params *layout) {
			Nat result = 0;
			for (Nat i = 0; i < layout->stackCount(); i++) {
				TypeDesc *desc = src->at(layout->stackAt(i)).type;
				result += roundUp(desc->size().size64(), nat(8));
			}
			return result;
		}

		static Nat pushValue(Listing *dest, const ParamInfo &p) {
			Nat size = p.type->size().size64();
			if (size <= 8) {
				*dest << push(p.src);
				return 8;
			}

			// We need to perform a memcpy-like operation (only for variables).
			if (p.src.type() != opVariable)
				throw InvalidValue(L"Can not pass non-variables larger than 8 bytes to functions.");

			Var src = p.src.var();
			Nat pushed = 0;

			// Last part:
			Nat last = size & 0x07;
			size -= last; // Now 'size' is a multiple of 8.
			if (last == 1) {
				*dest << push(byteRel(src, Offset(size)));
				pushed += 8;
			} else if (last <= 4) {
				*dest << push(intRel(src, Offset(size)));
				pushed += 8;
			} else /* last < 8 */ {
				*dest << push(longRel(src, Offset(size)));
				pushed += 8;
			}

			while (size >= 8) {
				size -= 8;
				*dest << push(longRel(src, Offset(size)));
				pushed += 8;
			}

			return pushed;
		}

		static Nat pushLea(Listing *dest, const ParamInfo &p) {
			assert(false, L"Not implemented yet!");
			return 0;
		}

		static Nat pushRef(Listing *dest, const ParamInfo &p) {
			assert(false, L"Not implemented yet!");
			return 0;
		}

		static Nat pushParams(Listing *dest, Array<ParamInfo> *src, Params *layout) {
			Nat pushed = 0;
			Nat size = stackParamsSize(src, layout);
			if (size & 0x0F) {
				// We need to push an additional word to the stack to keep alignment.
				*dest << push(natConst(0));
				pushed += 8;
			}

			// Push the parameters.
			for (Nat i = layout->stackCount(); i > 0; i--) {
				const ParamInfo &p = src->at(layout->stackAt(i - 1));
				if (p.ref == p.lea) {
					pushed += pushValue(dest, p);
				} else if (p.ref) {
					pushed += pushRef(dest, p);
				} else if (p.lea) {
					pushed += pushLea(dest, p);
				}
			}

			return pushed;
		}

		struct RegEnv {
			Listing *dest;
			Array<ParamInfo> *src;
			Params *layout;
			Array<Bool> *active;
			Array<Bool> *finished;
		};

		static void setRegister(RegEnv &env, Nat i);

		// Make sure any content inside 'reg' is used now, so that 'reg' can be reused for other purposes.
		static void vacateRegister(RegEnv &env, Reg reg) {
			for (Nat i = 0; i < env.layout->registerCount(); i++) {
				Param p = env.layout->registerAt(i);
				if (p == Param())
					continue;

				const Operand &src = env.src->at(p.id()).src;
				if (src.type() == opRegister && same(src.reg(), reg)) {
					// We need to set this register now, otherwise it will be destroyed!
					if (env.active->at(i)) {
						// Cycle detected. Push the register we should vacate onto the stack and
						// keep a note about that for later.
						*env.dest << push(src);
						env.active->at(i) = false;
					} else {
						setRegister(env, i);
					}
				}
			}
		}

		static void setRegisterVal(RegEnv &env, Reg target, Param param, const Operand &src) {
			if (param.offset() == 0 && src.size().size64() <= 8) {
				*env.dest << mov(asSize(target, src.size()), src);
			} else if (src.type() == opVariable) {
				Size s(param.size());
				*env.dest << mov(asSize(target, s), xRel(s, src.var(), Offset(param.offset())));
			} else {
				throw InvalidValue(L"Can not pass non-variables larger than 8 bytes to functions.");
			}
		}

		static void setRegisterLea(RegEnv &env, Reg target, Param param, const Operand &src) {
			assert(param.size() == 8);
			*env.dest << lea(asSize(target, Size::sPtr), src);
		}

		static void setRegisterRef(RegEnv &env, Reg target, Param param, const Operand &src) {
			assert(false, L"Not implemented yet!");
		}

		static void setRegister(RegEnv &env, Nat i) {
			Param param = env.layout->registerAt(i);
			// Empty?
			if (param == Param())
				return;
			// Already done?
			if (env.finished->at(i))
				return;

			Reg target = env.layout->registerSrc(i);
			ParamInfo &p = env.src->at(param.id());

			// See if 'target' contains something that is used by other parameters.
			env.active->at(i) = true;
			vacateRegister(env, target);
			if (!env.active->at(i)) {
				// This register is stored on the stack for now. Restore it before we continue!
				p.src = Operand(asSize(target, p.src.size()));
				*env.dest << pop(p.src);
			}
			env.active->at(i) = false;

			// Set the register.
			if (p.ref == p.lea)
				setRegisterVal(env, target, param, p.src);
			else if (p.ref)
				setRegisterRef(env, target, param, p.src);
			else if (p.lea)
				setRegisterLea(env, target, param, p.src);

			// Note that we're done.
			env.finished->at(i) = true;
		}

		static void setRegisters(Listing *dest, Array<ParamInfo> *src, Params *layout) {
			RegEnv env = {
				dest,
				src,
				layout,
				new (dest) Array<Bool>(layout->registerCount(), false),
				new (dest) Array<Bool>(layout->registerCount(), false),
			};

			for (Nat i = 0; i < layout->registerCount(); i++) {
				setRegister(env, i);
			}
		}

		static void returnPrimitive(Listing *dest, PrimitiveDesc *p, Instr *instr) {
			switch (p->v.kind()) {
			case primitive::none:
				break;
			case primitive::integer:
			case primitive::pointer:
				if (instr->dest().type() == opRegister && same(instr->dest().reg(), ptrA)) {
				} else {
					*dest << mov(instr->dest(), asSize(ptrA, instr->dest().size()));
				}
				break;
			case primitive::real:
				*dest << mov(instr->dest(), asSize(xmm0, instr->dest().size()));
				break;
			}
		}

		static void returnSimple(Listing *dest, primitive::PrimitiveKind k, nat &i, nat &r, Offset offset, Size totalSize) {
			static const Reg intReg[2] = { ptrA, ptrD };
			static const Reg realReg[2] = { xmm0, xmm1 };
			// Make sure not to overwrite any crucial memory region...
			Size size(min(totalSize.size64() - offset.v64(), 8));

			switch (k) {
			case primitive::none:
				break;
			case primitive::integer:
			case primitive::pointer:
				*dest << mov(xRel(size, ptrSi, offset), asSize(intReg[i++], size));
				break;
			case primitive::real:
				*dest << mov(xRel(size, ptrSi, offset), asSize(realReg[r++], size));
				break;
			}
		}

		static void returnSimple(Listing *dest, Result *result, Size size) {
			nat i = 0;
			nat r = 0;
			returnSimple(dest, result->part1, i, r, Offset(), size);
			returnSimple(dest, result->part2, i, r, Offset::sPtr, size);
		}

		void RemoveInvalid::fnCallTfm(Listing *dest, Instr *instr, Nat line) {
			if (!as<TypeInstr>(instr)) {
				TODO(L"Remove me!"); return;
				throw InvalidValue(L"Using a fnCall that was not created properly.");
			}

			TODO(L"COMPLETE ME!");
			Block block;
			Array<Var> *copies;
			TypeDesc *result = ((TypeInstr *)instr)->type;
			Result *resultLayout = code::x64::result(result);
			bool complex = hasComplex(params);

			if (complex) {
				block = dest->createBlock(currentPart);
				copies = new (this) Array<Var>(params->count(), Var());
				*dest << begin(block);

				Part part = block;
				for (Nat i = 0; i < params->count(); i++) {
					const ParamInfo &param = params->at(i);

					if (ComplexDesc *c = as<ComplexDesc>(param.type)) {
						part = dest->createPart(part);
						Var v = dest->createVar(part, c->size(), c->dtor);
						copies->at(i) = v;

						// Call the copy constructor.
						*dest << lea(ptrDi, v);
						*dest << mov(ptrSi, param.src); // TODO: Load from a safe place.
						*dest << call(c->ctor, valVoid());
						*dest << begin(part);
					}
				}
			}

			// Do we need a hidden parameter?
			if (resultLayout->memory) {
				// We want to pass a reference to 'src'.
				params->insert(0, ParamInfo(ptrDesc(engine()), instr->dest(), false, true));
			}

			Params *layout = paramLayout(params);

			// Push parameters on the stack. This is a 'safe' operation since it does not destroy any registers.
			Nat pushed = pushParams(dest, params, layout);

			// Assign parameters to registers.
			setRegisters(dest, params, layout);

			// Call the function.
			*dest << call(instr->src(), valVoid());

			// Handle the return value if required.
			if (PrimitiveDesc *p = as<PrimitiveDesc>(result)) {
				returnPrimitive(dest, p, instr);
			} else if (!resultLayout->memory) {
				*dest << lea(ptrSi, instr->dest());
				returnSimple(dest, resultLayout, result->size());
			}

			// Pop the stack.
			if (pushed > 0)
				*dest << add(ptrStack, ptrConst(pushed));

			if (complex) {
				// TODO: Preserve registers as required!
				*dest << end(block);
			}

			params->clear();
		}

		void RemoveInvalid::fnCallRefTfm(Listing *dest, Instr *instr, Nat line) {
			params->clear();
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
				*dest << instr;
				return;
			}

			Size size = instr->dest().size();

			// We need to store the value in 'cl'. See if 'dest' is also 'cl' or 'rcx'.
			if (instr->dest().type() == opRegister && same(instr->dest().reg(), rcx)) {
				// Yes. We need to swap things around quite a lot!
				Reg reg = asSize(unusedReg(used->at(line)), size);
				*dest << mov(reg, instr->dest());
				*dest << mov(cl, instr->src());
				*dest << instr->alter(reg, cl);
				*dest << mov(instr->dest(), reg);
			} else {
				// We do not need to do that at least!
				Reg reg = asSize(unusedReg(used->at(line)), Size::sLong);
				*dest << mov(reg, rcx);
				*dest << mov(cl, instr->src());
				*dest << instr->alterSrc(cl);
				*dest << mov(rcx, reg);
			}
		}

		void RemoveInvalid::shrTfm(Listing *dest, Instr *instr, Nat line) {
			shlTfm(dest, instr, line);
		}

		void RemoveInvalid::sarTfm(Listing *dest, Instr *instr, Nat line) {
			shlTfm(dest, instr, line);
		}

		void RemoveInvalid::idivTfm(Listing *dest, Instr *instr, Nat line) {
			RegSet *used = new (this) RegSet(*this->used->at(line));
			const Operand &op = instr->dest();
			bool small = op.size() == Size::sByte;

			// If 'src' is a constant, we need to move it into a register.
			if (instr->src().type() == opConstant) {
				Reg r = asSize(unusedReg(used), instr->src().size());
				*dest << mov(r, instr->src());
				instr = instr->alterSrc(r);
			}

			// Make sure ptrD can be trashed (not needed if we're working with bytes).
			Reg oldD = noReg;
			if (!small && used->has(ptrD)) {
				oldD = asSize(unusedReg(used), Size::sPtr);
				*dest << mov(oldD, ptrD);
				used->put(oldD);
			}

			if (op.type() == opRegister && same(op.reg(), ptrA)) {
				// Supported!
				*dest << instr;
			} else {
				// We need to put op into 'ptrA'.
				Reg oldA = noReg;
				if (used->has(ptrA)) {
					oldA = asSize(unusedReg(used), Size::sPtr);
					*dest << mov(oldA, ptrA);
					used->put(oldA);
				}

				Reg destA = asSize(ptrA, op.size());
				*dest << mov(destA, op);
				if (instr->src().type() == opRegister && same(instr->src().reg(), ptrA)) {
					*dest << instr->alter(destA, asSize(oldA, instr->src().size()));
				} else {
					*dest << instr->alterDest(destA);
				}
				*dest << mov(op, destA);

				if (oldA != noReg) {
					*dest << mov(ptrA, oldA);
				}
			}

			if (oldD != noReg) {
				*dest << mov(ptrD, oldD);
			}
		}

		void RemoveInvalid::udivTfm(Listing *dest, Instr *instr, Nat line) {
			idivTfm(dest, instr, line);
		}

		void RemoveInvalid::imodTfm(Listing *dest, Instr *instr, Nat line) {
			RegSet *used = new (this) RegSet(*this->used->at(line));
			const Operand &op = instr->dest();
			bool small = op.size() == Size::sByte;

			// If 'src' is a constant, we need to move it into a register.
			if (instr->src().type() == opConstant) {
				Reg r = asSize(unusedReg(used), instr->src().size());
				*dest << mov(r, instr->src());
				instr = instr->alterSrc(r);
			}

			// Make sure ptrD can be trashed (unless we're working with 8 bit numbers).
			Reg oldD = noReg;
			if (!small && used->has(ptrD)) {
				oldD = asSize(unusedReg(used), Size::sPtr);
				*dest << mov(oldD, ptrD);
				used->put(oldD);
			}

			// We need to put op into 'ptrA'.
			Reg oldA = noReg;
			if (used->has(ptrA)) {
				oldA = asSize(unusedReg(used), Size::sPtr);
				*dest << mov(oldA, ptrA);
				used->put(oldA);
			}

			Reg destA = asSize(ptrA, op.size());
			if (op.type() != opRegister || op.reg() != destA)
				*dest << mov(destA, op);

			if (instr->src().type() == opRegister && same(instr->src().reg(), ptrA)) {
				*dest << instr->alter(destA, asSize(oldA, instr->src().size()));
			} else {
				*dest << instr->alterDest(destA);
			}

			Reg destD = asSize(ptrD, op.size());
			if (small) {
				// We need to shift the register a bit (we are not able to access AH in this implementation).
				*dest << shr(eax, byteConst(8));
				destD = al;
			}

			if (op.type() != opRegister || op.reg() != destD)
				*dest << mov(op, destD);

			// Restore registers.
			if (oldA != noReg) {
				*dest << mov(ptrA, oldA);
			}
			if (oldD != noReg) {
				*dest << mov(ptrD, oldD);
			}
		}

		void RemoveInvalid::umodTfm(Listing *dest, Instr *instr, Nat line) {
			imodTfm(dest, instr, line);
		}

	}
}
