#include "stdafx.h"
#include "InlineParams.h"
#include "Exception.h"

namespace storm {

	InlineParams::InlineParams(CodeGen *state, Array<code::Operand> *params, CodeResult *result) :
		state(state), originalParams(params), params(params), result(result) {}

	void InlineParams::deepCopy(CloneEnv *env) {
		clone(state, env);
		clone(originalParams, env);
		clone(params, env);
		clone(result, env);
	}

	void InlineParams::spillParams() {
		params = spillRegisters(state, params);
	}

	void InlineParams::allocRegs(Nat id0) {
		Nat d[] = { id0 };
		allocRegs(id0, d, 1);
	}

	void InlineParams::allocRegs(Nat id0, Nat id1) {
		Nat d[] = { id0, id1 };
		allocRegs(id0, d, 2);
		allocRegs(id1, d, 2);
	}

	void InlineParams::allocRegs(Nat id0, Nat id1, Nat id2) {
		Nat d[] = { id0, id1, id2 };
		allocRegs(id0, d, 3);
		allocRegs(id1, d, 3);
		allocRegs(id2, d, 3);
	}

	void InlineParams::allocRegs(Nat id, Nat *avoid, Nat avoidCount) {
		using namespace code;

		Operand p = params->at(id);
		if (p.type() == opRegister) {
			// Nothing to do.
		} else if (p.type() == code::opRelative) {
			// Just read the value into the register.
			Reg r = asSize(p.reg(), p.size());
			*state->l << mov(r, p);
			replace(id, r);
		} else {
			Reg free = asSize(findReg(avoid, avoidCount), p.size());
			*state->l << mov(free, p);
			replace(id, free);
		}
	}

	static Bool contains(Nat *array, Nat count, Nat elem) {
		for (Nat i = 0; i < count; i++)
			if (array[i] == elem)
				return true;

		return false;
	}

	code::Reg InlineParams::findReg(Nat *avoid, Nat avoidCount) {
		using namespace code;

		{
			// See if one of the registers are free already.
			Reg candidates[] = { ptrA, ptrB, ptrC };
			for (Nat i = 0; i < params->count(); i++) {
				const Operand &p = params->at(i);
				if (p.hasRegister()) {
					for (Nat j = 0; j < ARRAY_COUNT(candidates); j++) {
						if (same(candidates[j], p.reg()))
							candidates[j] = noReg;
					}
				}
			}

			for (Nat i = 0; i < ARRAY_COUNT(candidates); i++) {
				if (candidates[i] != noReg)
					return candidates[i];
			}
		}

		// If we get here, we did not find a free register. This means we shall pick a register that
		// is used by another parameter and use that.
		Reg chosen = noReg;
		Nat id = 0;
		for (Nat i = 0; i < params->count(); i++) {
			if (contains(avoid, avoidCount, i))
				continue;

			if (params->at(i).hasRegister()) {
				chosen = noReg;
				id = i;
				break;
			}
		}

		if (chosen == noReg) {
			// Should not happen as we only allow assigning 3 parameters to register.
			throw InternalError(L"Unable to find a free register.");
		}

		// Move the old value out of the way so that we can reuse the register.
		Var mem = state->l->createVar(state->block, params->at(id).size());
		*state->l << mov(mem, params->at(id));
		replace(id, mem);

		return chosen;
	}

	void InlineParams::replace(Nat id, const code::Operand &op) {
		if (params == originalParams)
			params = new (params) Array<code::Operand>(*params);
		params->at(id) = op;
	}

}
