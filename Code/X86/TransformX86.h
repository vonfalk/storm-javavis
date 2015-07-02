#pragma once

#ifdef X86
#include "Listing.h"
#include "UsedRegisters.h"

namespace code {
	namespace machineX86 {

		/**
		 * Transform unsupported op-codes into sequences of other op-codes.
		 * This step is free to transform the code in (almost) any way.
		 */
		class Transform : public Transformer {
		public:
			Transform(const Listing &from, Arena &arena);

			// Arena.
			Arena &arena;

			// Remember unused registers.
			UsedRegisters registers;

			// Get a good unused register at 'line'.
			Register unusedReg(nat line) const;

			// Preserve a number of registers.
			vector<Register> preserve(const vector<Register> &regs, nat line, Listing &to) const;

			// Restore registers previously preserved.
			void restore(const vector<Register> &regs, const vector<Register> &saved, nat line, Listing &to) const;

		protected:
			virtual void transform(Listing &to, nat line);

		private:
			// Preserve a single register
			Register preserve(Register r, const Registers &used, Listing &to) const;

			// Restore a single register
			void restore(Register r, Register saved, const Registers &used, Listing &to) const;

			// Pick an unused register.
			Register unusedReg(const Registers &used) const;
		};

		typedef void (*TransformFn)(const Transform &, Listing &, nat);

		// Transform functions executed. Assigned to op-codes in MachineCodeX86.cpp
		// immRegTfm is a special case.
		void immRegTfm(const Transform &tfm, Listing &to, nat line);
		void leaTfm(const Transform &tfm, Listing &to, nat line);
		void mulTfm(const Transform &tfm, Listing &to, nat line);
		void idivTfm(const Transform &tfm, Listing &to, nat line);
		void udivTfm(const Transform &tfm, Listing &to, nat line);
		void imodTfm(const Transform &tfm, Listing &to, nat line);
		void umodTfm(const Transform &tfm, Listing &to, nat line);
		void setCondTfm(const Transform &tfm, Listing &to, nat line);
		void shlTfm(const Transform &tfm, Listing &to, nat line);
		void shrTfm(const Transform &tfm, Listing &to, nat line);
		void sarTfm(const Transform &tfm, Listing &to, nat line);
		void icastTfm(const Transform &tfm, Listing &to, nat line);
		void ucastTfm(const Transform &tfm, Listing &to, nat line);
		void addRefTfm(const Transform &tfm, Listing &to, nat line);
		void releaseRefTfm(const Transform &tfm, Listing &to, nat line);
		void callFloatTfm(const Transform &tfm, Listing &to, nat line);
		void retFloatTfm(const Transform &tfm, Listing &to, nat line);
		void fnCallFloatTfm(const Transform &tfm, Listing &to, nat line);

	}
}

#endif
