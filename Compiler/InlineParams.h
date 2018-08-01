#pragma once
#include "Core/Array.h"
#include "CodeGen.h"

namespace storm {
	STORM_PKG(core.lang);

	/**
	 * Parameters passed to functions generating inline code.
	 *
	 * Since parameters may reside in registers, any code that uses registers needs to be careful
	 * when using registers so that no parameters are accidentally overwritten. For this reason,
	 * functions that are using registers internally should call either 'allocRegs' or
	 * 'spillParameters' before any parameters are accessed.
	 *
	 * Parameters are then accessed by calling the 'parameter' function, or if the parameter is
	 * known to be located in a register, the function 'regParameter' may be used for convenience.
	 */
	class InlineParams {
		STORM_VALUE;
	public:
		STORM_CTOR InlineParams(CodeGen *state, Array<code::Operand> *params, CodeResult *result);

		CodeGen *state;
		CodeResult *result;

		void STORM_FN deepCopy(CloneEnv *env);

		// Spill all parameters in registers to memory. Call before accessing any parameters.
		void STORM_FN spillParams();

		// Make sure the desired parameters are located in some register. Other parameters are left
		// as they are unless we need to steal the register they are currently using.
		void STORM_FN allocRegs(Nat id0);
		void STORM_FN allocRegs(Nat id0, Nat id1);
		void STORM_FN allocRegs(Nat id0, Nat id1, Nat id2);

		// Get a parameter.
		code::Operand STORM_FN param(Nat id) const { return params->at(id); }

		// Get a parameter known to be placed in a register.
		code::Reg STORM_FN regParam(Nat id) const { return param(id).reg(); }

		// Get the original location of a parameter, suitable to use when calling 'suggest' since
		// 'suggest' only attempts to reuse parameters in memory, which is exactly the case for when
		// this is reliable.
		code::Operand STORM_FN originalParam(Nat id) const { return originalParams->at(id); }

	private:
		// Original location of all parameters. Suitable to use with 'suggest'.
		Array<code::Operand> *originalParams;

		// The parameters. Updated to match the desired layout.
		Array<code::Operand> *params;

		// Helper for 'allocRegs'.
		void allocRegs(Nat id, Nat *avoid, Nat avoidCount);

		// Find a free register to put a parameter in. Do not consider parameters in 'avoid' as candidates.
		code::Reg findReg(Nat *avoid, Nat avoidCount);

		// Replace parameter 'id' with 'Operand'.
		void replace(Nat id, const code::Operand &op);
	};

}
