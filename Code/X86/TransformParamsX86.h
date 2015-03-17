#pragma once

#ifdef X86
#include "MachineCodeX86.h"
#include "Seh.h"
#include "VariableX86.h"

namespace code {
	namespace machineX86 {

		/**
		 * Transform parameters into relative addresses. Make sure not to add more used registers
		 * in this step, since prolog and epilog are transformed badly in that case.
		 */
		class TfmParams : public Transformer {
		public:
			TfmParams(const Listing &from, const Binary *owner);

			// State during transform:

			// Function parameters yet to be called.
			struct FnParam {
				Value param;
				Value copy;
			};
			vector<FnParam> fnParams;

			// Current part.
			Part currentPart;

			// Registers needed to be preserved for the current instruction
			Registers preserve;
			// Owner object.
			const Binary *owner;

			// Register usage
			UsedRegisters registers;

			// Saved registers. (only the 32-bit versions).
			Registers savedRegisters;
			nat savedRegisterCount;

			// Variables
			Offsets vars;

			virtual void before(Listing &to);
			virtual void transform(Listing &to, nat line);
			virtual void after(Listing &to);

			// Look up variables in instructions.
			void lookupVars(Instruction &instr) const;
			void lookupVars(Listing &to, Instruction instr) const;

			// Replace the value 'v' if it is a variable. Returns true if 'v' was altered.
			bool lookupVar(Value &v) const;
			inline Value lookup(const Variable &v, int offset) const { return vars.variable(v, offset); }
		};

		// Transform function calls into regular asm-instructions.
		void fnParamTfm(Listing &to, TfmParams &params, const Instruction &instr);
		void fnCallTfm(Listing &to, TfmParams &params, const Instruction &instr);

		// Transform prolog and epilog.
		void prologTfm(Listing &to, TfmParams &params, const Instruction &instr);
		void epilogTfm(Listing &to, TfmParams &params, const Instruction &instr);

		// Transform block start and end.
		void beginBlockTfm(Listing &to, TfmParams &params, const Instruction &instr);
		void endBlockTfm(Listing &to, TfmParams &params, const Instruction &instr);

	}
}

#endif
