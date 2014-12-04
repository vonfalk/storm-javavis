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
			Transform(const Listing &from);

			// Remember unused registers.
			UsedRegisters registers;

			// Get a good unused register at 'line'.
			Register unusedReg(nat line) const;

		protected:
			virtual void transform(Listing &to, nat line);
		};

		typedef void (*TransformFn)(const Transform &, Listing &, nat);

		// Transform functions executed. Assigned to op-codes in MachineCodeX86.cpp
		// immRegTfm is a special case.
		void immRegTfm(const Transform &tfm, Listing &to, nat line);
		void mulTfm(const Transform &tfm, Listing &to, nat size);

	}
}

#endif
