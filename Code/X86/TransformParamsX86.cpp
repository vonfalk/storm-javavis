#include "stdafx.h"
#include "TransformParamsX86.h"
#ifdef X86
#include "VariableX86.h"
#include "Listing.h"
#include "Instruction.h"
#include "Exception.h"
#include "Seh.h"
#include "OpTable.h"

namespace code {
	namespace machineX86 {

#define TRANSFORM(x) { op::x, &machineX86::x ## Tfm }

		typedef void (*TransformParFn)(Listing &, machineX86::TfmParams &, const Instruction &);

		static OpEntry<TransformParFn> transformMap[] = {
			TRANSFORM(fnParam),
			TRANSFORM(fnCall),
			TRANSFORM(prolog),
			TRANSFORM(epilog),
			TRANSFORM(beginBlock),
			TRANSFORM(endBlock),
		};

		//////////////////////////////////////////////////////////////////////////
		// Params
		//////////////////////////////////////////////////////////////////////////

		TfmParams::TfmParams(const Listing &from, const Binary *owner) : Transformer(from), owner(owner), registers(from) {
			Registers r = registers.allUsed();
			savedRegisterCount = 0;

			// Handle 64-bit registers.
			add64(r);

			vector<Register> allRegisters = regsBase();
			Registers notPreserved(regsNotPreserved());

			for (nat i = 0; i < allRegisters.size(); i++) {
				Register reg = allRegisters[i];
				if (r.contains(reg)) {
					if (!notPreserved.contains(reg)) {
						savedRegisters += reg;
						savedRegisterCount++;
					}
				}
			}

			vars.init(savedRegisterCount, from.frame);
		}

		void TfmParams::before(Listing &to) {}

		void TfmParams::transform(Listing &to, nat line) {
			static OpTable<TransformParFn> transforms(transformMap, ARRAY_SIZE(transformMap));

			Instruction i(from[line]);
			lookupVars(i);

			TransformParFn tfm = transforms[i.op()];

			if (tfm) {
				(*tfm)(to, *this, i);
			} else {
				to << i;
			}
		}

		void TfmParams::after(Listing &to) {
			to << to.metadata();
			Meta::write(to, to.frame, vars);
		}

		//////////////////////////////////////////////////////////////////////////
		// Variable lookup
		//////////////////////////////////////////////////////////////////////////

		bool TfmParams::lookupVar(Value &v) const {
			if (v.type() == Value::tVariable) {
				Variable var = v.variable();
				if (!from.frame.accessible(currentPart, var))
					throw VariableUseError(var, currentPart);

				v = vars.variable(var, v.offset().current());
				return true;
			}
			return false;
		}

		void TfmParams::lookupVars(Instruction &instr) const {
			Value src = instr.src();
			Value dest = instr.dest();

			bool altered = lookupVar(src);
			altered |= lookupVar(dest);

			if (altered)
				instr = instr.altered(dest, src);
		}

		void TfmParams::lookupVars(Listing &to, Instruction instr) const {
			lookupVars(instr);
			to << instr;
		}


		//////////////////////////////////////////////////////////////////////////
		// Initialize and destroy blocks
		//////////////////////////////////////////////////////////////////////////

		// Inserts zeros in the memory at 'start', and 'size' bytes after. May round 'size' to the next multiple of 4. (eg. size = 3, size = 6)
		static void zeroMem(Listing &to, Value start, nat size) {
			Register base;
			cpuInt offset = 0;

			if (size == 0)
				size = sizeof(cpuNat);

			switch (start.type()) {
				case Value::tRelative:
					base = start.reg();
					offset = start.offset().current();
					break;
				default:
					assert(false); // Not supported
					break;
			}

			for (nat i = 0; i < size; i += sizeof(cpuNat)) {
				nat remaining = size - i;
				if (remaining == 1) {
					to << code::mov(byteRel(base, Offset(cpuInt(i) + offset)), byteConst(0));
				} else {
					to << code::mov(intRel(base, Offset(cpuInt(i) + offset)), natConst(0));
				}
			}
		}

		static void initBlock(Listing &to, TfmParams &params, Part p) {
			const code::Frame &frame = to.frame;

			if (params.currentPart != frame.prev(p)) {
				throw BlockBeginError(L"Can not begin " + toS(Value(p)) + L" unless the current is "
									+ toS(Value(frame.prev(p))) + L". We are now in "
									+ toS(Value(params.currentPart)));
			}


			params.currentPart = p;

			Block b = frame.asBlock(p);
			if (b != Block::invalid) {

				// Initialize variables to zero, the entire block!
				for (Part c = b; c != Part::invalid; c = frame.next(c)) {
					vector<Variable> vars = frame.variables(c);
					for (nat i = 0; i < vars.size(); i++) {
						Variable var = vars[i];

						if (!frame.isParam(var)) {
							zeroMem(to, params.lookup(var, 0), var.size().current());
						}
					}
				}
			}

			to << code::mov(params.vars.partId(), natConst(p.getId()));
		}

		static void destroyBlock(Listing &to, TfmParams &params, Part p, bool preserveEax) {
			const code::Frame &frame = to.frame;

			if (params.currentPart != p) {
				assert(false);
				throw BlockEndError();
			}

			bool pushedEax = false;
			bool pushedEdx = false;

			// Call destructor on relevant elements...
			vector<Variable> vars = frame.variables(p);
			for (nat i = 0; i < vars.size(); i++) {
				Variable var = vars[i];

				Value dtor = frame.freeFn(var);
				FreeOpt on = frame.freeOpt(var);

				if (dtor.type() != Value::tNone && (on & freeOnBlockExit) == freeOnBlockExit) {
					if (preserveEax && !pushedEax) {
						to << code::push(eax);
						pushedEax = true;
					}

					if (preserveEax && params.preserve.contains(edx) && !pushedEdx) {
						to << code::push(edx);
						pushedEdx = true;
					}

					if (on & freePtr) {
						Instruction i = code::lea(ptrA, ptrRel(var));
						params.lookupVars(i);
						to << i;

						i = code::fnParam(ptrA);
						params.lookupVars(i);
						fnParamTfm(to, params, i);
					} else {
						Instruction i = code::fnParam(var);
						params.lookupVars(i);
						fnParamTfm(to, params, i);
					}

					Instruction i = code::fnCall(dtor, Size());
					params.lookupVars(i);
					fnCallTfm(to, params, i);

					zeroMem(to, params.lookup(var, 0), var.size().current());
				}
			}

			if (pushedEdx)
				to << code::pop(edx);

			if (pushedEax)
				to << code::pop(eax);

			Part prev = frame.prev(p);
			params.currentPart = prev;
			to << code::mov(params.vars.partId(), natConst(prev.getId()));
		}

		void fnParamTfm(Listing &to, TfmParams &params, const Instruction &instr) {
			// Remember the parameter...
			TfmParams::FnParam p = { instr.src(), instr.dest() };
			params.fnParams.push_back(p);
		}

		struct ParamOffset {
			nat fromStart;
			nat valueId;
		};

		void fnCallTfm(Listing &to, TfmParams &params, const Instruction &instr) {
			// Simple cdecl call.
			vector<TfmParams::FnParam> &fnParams = params.fnParams;

			// Keep track of any copy-functions we need to call!
			vector<ParamOffset> copies;
			nat totalSize = 0;

			for (nat i = fnParams.size(); i > 0; i--) {
				TfmParams::FnParam &p = fnParams[i-1];
				if (p.copy == Value()) {
					// Nothing special...
					to << code::push(p.param);
					totalSize += sizeof(cpuNat);
				} else {
					// Reserve some empty space on the stack...
					nat s = roundUp(p.param.size().current(), sizeof(cpuNat));
					totalSize += s;
					to << code::sub(ptrStack, natPtrConst(s));
					ParamOffset par = { totalSize, i - 1 };
					copies.push_back(par);
				}
			}

			// Now, we can destroy all registers that are not preserved across function calls!
			for (nat i = 0; i < copies.size(); i++) {
				ParamOffset &offset = copies[i];
				TfmParams::FnParam &src = fnParams[offset.valueId];

				// Note: sizeof(cpuNat) is because we need to skip the first param of the copy function.
				Offset destOffset(totalSize - offset.fromStart + sizeof(cpuNat));

				// Copy!
				to << code::lea(ptrA, ptrRel(src.param.reg(), src.param.offset()));
				to << code::push(ptrA);
				to << code::lea(ptrA, ptrRel(ptrStack, destOffset));
				to << code::push(ptrA);
				to << code::call(src.copy, Size());
				to << code::add(ptrStack, natPtrConst(2 * sizeof(cpuNat)));
			}

			to << code::call(instr.src(), instr.dest().size());

			if (totalSize > 0)
				to << code::add(ptrStack, natPtrConst(totalSize));

			fnParams.clear();
		}


		void prologTfm(Listing &to, TfmParams &params, const Instruction &instr) {

			// Set up stack frame.
			to << code::push(ptrFrame);
			to << code::mov(ptrFrame, ptrStack);

			to << code::push(natConst(0));
			to << code::push(natConst(cpuNat(params.owner)));

			if (to.frame.exceptionHandlerNeeded()) {
				// Set up the SEH handler for this frame.
				// to << code::threadLocal() << code::push(natPtrConst(Nat(&x86SafeSEH)));
				to << code::push(natPtrConst(Nat(&x86SafeSEH)));
				to << code::threadLocal() << code::push(ptrRel(noReg));
				to << code::threadLocal() << code::mov(ptrRel(noReg), ptrStack);
			}

			// Save any registers we need to preserve.
			for (Registers::iterator i = params.savedRegisters.begin(); i != params.savedRegisters.end(); ++i) {
				to << code::push(*i);
			}

			nat stackVarSize = params.vars.maxSize();
			assert(stackVarSize % 4 == 0);
			if (stackVarSize)
				to << code::sub(ptrStack, natPtrConst(stackVarSize));

			// Initialize the root block.
			initBlock(to, params, Frame::root());
		}

		void epilogTfm(Listing &to, TfmParams &params, const Instruction &instr) {
			// Destroy blocks, keep the state in "params", we may return early!
			Part currentPart = params.currentPart;

			for (Part c = currentPart; c != Block::invalid; c = to.frame.prev(c)) {
				destroyBlock(to, params, c, true);
			}

			// Restore any previously preserved registers.
			nat id = 0;
			for (Registers::iterator i = params.savedRegisters.begin(); i != params.savedRegisters.end(); ++i, id++) {
				int offset = params.vars.preservedOffset() - id*sizeof(cpuNat);
				to << code::mov(*i, intRel(ptrFrame, Offset(offset)));
			}

			// Restore
			params.currentPart = currentPart;

			if (to.frame.exceptionHandlerNeeded()) {
				// Remove the SEH handler.

				// Find the previous handler (relative to ebp, if esp is not right)
				to << code::mov(edx, intRel(ptrFrame, Offset(-16)));
				// Restore it...
				to << code::threadLocal() << mov(intRel(noReg), edx);
			}

			to << code::mov(ptrStack, ptrFrame);
			to << code::pop(ptrFrame);

		}

		void beginBlockTfm(Listing &to, TfmParams &params, const Instruction &instr) {
			initBlock(to, params, instr.src().part());
		}

		void endBlockTfm(Listing &to, TfmParams &params, const Instruction &instr) {
			Part target = instr.src().part();
			Part start = params.currentPart;

			while (params.currentPart != target) {
				if (params.currentPart == Part::invalid) {
					PLN("Block " << Value(target) << " is not a parent of " << Value(start));
					throw BlockEndError();
				}

				// updates 'currentPart'.
				destroyBlock(to, params, params.currentPart, false);
			}

			// Destroy the one we notified as well.
			destroyBlock(to, params, params.currentPart, false);
		}

	}
}

#endif
