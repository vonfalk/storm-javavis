#include "stdafx.h"

#ifdef X86
#include "VariableX86.h"
#include "Utils/Math.h"

namespace code {
	namespace machineX86 {

#ifdef SEH
		// Specific for SEH.
		static int baseOffset(nat preservedRegisters, const Frame &frame) {
			int offset = preservedRegisters * sizeof(cpuNat);
			// current block and owner TODO: Really needed?
			offset += 2 * sizeof(cpuNat);
			// if using SEH, 2 more
			if (frame.exceptionHandlerNeeded())
				offset += 2 * sizeof(cpuNat);
			return -offset;
		}
#endif

		void Offsets::init(nat preservedRegisters, const Frame &frame) {
			vector<Variable> vars = frame.allVariables();
			off = vector<int>(vars.size(), 0); // All values begin un-initialized.
			maxSz = 0;

			for (nat i = 0; i < vars.size(); i++) {
				const Variable &var = vars[i];

				if (frame.isParam(var)) {
					updateParamOffset(var, frame);
				} else {
					updateVarOffset(var, frame, preservedRegisters);
				}
			}

			int base = baseOffset(preservedRegisters, frame);
			for (nat i = 0; i < off.size(); i++) {
				int offset = off[i];
				int size = base - offset;

				if (size > 0)
					maxSz = max(maxSz, nat(size));
			}
		}

		int Offsets::updateVarOffset(const Variable &var, const Frame &frame, nat savedReg) {
			nat id = var.getId();

			// Already computed?
			if (off[id] != 0)
				return off[id];

			// Find the variable right before us in the frame.
			Variable p = frame.prev(var);
			nat size = var.size().currentSize();
			roundUp(size, sizeof(cpuNat));

			int offset;
			if (p == Variable::invalid) {
				offset = baseOffset(savedReg, frame);
			} else {
				offset = updateVarOffset(p, frame, savedReg);
			}

			offset -= size;
			off[id] = offset;
			return offset;
		}

		int Offsets::updateParamOffset(const Variable &var, const Frame &frame) {
			nat id = var.getId();

			// Already computed?
			if (off[id] != 0)
				return off[id];

			Variable p = frame.prev(var);

			int offset;
			if (p == Variable::invalid) {
				// return address and ebp.
				offset = 2 * sizeof(cpuNat);
			} else {
				nat size = p.size().currentSize();
				roundUp(size, sizeof(cpuNat));

				offset = updateParamOffset(p, frame);
				offset += size;
			}

			off[id] = offset;
			return offset;
		}

		int Offsets::offset(Variable v) const {
			return off[v.getId()];
		}

		Value Offsets::variable(Variable v, int off) const {
			return xRel(v.size(), ptrFrame, Offset(offset(v) + off));
		}

		Value Offsets::blockId() const {
			return intRel(ptrFrame, Offset(-4));
		}

		Value Offsets::blockPtr() const {
			return intRel(ptrFrame, Offset(-8));
		}

		//////////////////////////////////////////////////////////////////////////
		// Write metadata
		//////////////////////////////////////////////////////////////////////////


		void Meta::write(Listing &to, const Frame &frame, const Offsets &offsets) {
			vector<Variable> variables = frame.allVariables();
			to << dat(intConst(variables.size()));

			for (nat i = 0; i < variables.size(); i++) {
				Variable var = variables[i];
				int offset = offsets.offset(var);
				Value freeFn = frame.freeFn(var);

				if (freeFn.empty())
					freeFn = natPtrConst(0);
				assert(freeFn.size() == Size::sPtr);

				to << dat(intConst(offset));
				to << dat(freeFn);
			}
		}
	}
}


#endif
