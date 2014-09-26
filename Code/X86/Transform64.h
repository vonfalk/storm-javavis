#pragma once

#ifdef X86
#include "Listing.h"

namespace code {
	namespace machineX86 {

		class Transform64 : public Transformer {
		public:
			Transform64(const Listing &from);

			virtual void transform(Listing &to, nat line);

			// Transforms
			void movTfm(Listing &to, const Instruction &instr);
			void addTfm(Listing &to, const Instruction &instr);
			void adcTfm(Listing &to, const Instruction &instr);
			void orTfm(Listing &to, const Instruction &instr);
			void andTfm(Listing &to, const Instruction &instr);
			void subTfm(Listing &to, const Instruction &instr);
			void sbbTfm(Listing &to, const Instruction &instr);
			void xorTfm(Listing &to, const Instruction &instr);
			void cmpTfm(Listing &to, const Instruction &instr);

			void pushTfm(Listing &to, const Instruction &instr);
			void popTfm(Listing &to, const Instruction &instr);
		};

	}
}

#endif