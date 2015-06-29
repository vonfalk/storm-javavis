#pragma once

#ifdef X86
#include "Listing.h"
#include "UsedRegisters.h"

namespace code {
	namespace machineX86 {

		class Transform64 : public Transformer {
		public:
			Transform64(const Listing &from);

			// Information about used registers.
			UsedRegisters registers;

			virtual void transform(Listing &to, nat line);

			// Transforms
			void movTfm(Listing &to, const Instruction &instr, const Registers &used);
			void addTfm(Listing &to, const Instruction &instr, const Registers &used);
			void adcTfm(Listing &to, const Instruction &instr, const Registers &used);
			void orTfm(Listing &to, const Instruction &instr, const Registers &used);
			void andTfm(Listing &to, const Instruction &instr, const Registers &used);
			void subTfm(Listing &to, const Instruction &instr, const Registers &used);
			void sbbTfm(Listing &to, const Instruction &instr, const Registers &used);
			void xorTfm(Listing &to, const Instruction &instr, const Registers &used);
			void cmpTfm(Listing &to, const Instruction &instr, const Registers &used);
			void mulTfm(Listing &to, const Instruction &instr, const Registers &used);
			void idivTfm(Listing &to, const Instruction &instr, const Registers &used);
			void udivTfm(Listing &to, const Instruction &instr, const Registers &used);
			void imodTfm(Listing &to, const Instruction &instr, const Registers &used);
			void umodTfm(Listing &to, const Instruction &instr, const Registers &used);

			void fnParamTfm(Listing &to, const Instruction &instr, const Registers &used);
			void pushTfm(Listing &to, const Instruction &instr, const Registers &used);
			void popTfm(Listing &to, const Instruction &instr, const Registers &used);
		};

	}
}

#endif
