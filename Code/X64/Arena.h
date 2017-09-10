#pragma once
#include "../Arena.h"

namespace code {
	namespace x64 {
		STORM_PKG(core.asm.x64);

		/**
		 * Arena for X86-64 (TODO: factor windows/unix versions of this one!)
		 */
		class Arena : public code::Arena {
			STORM_CLASS;
		public:
			// Create.
			STORM_CTOR Arena();

			/**
			 * Transform.
			 */

			virtual Listing *STORM_FN transform(Listing *src, Binary *owner) const;
			virtual void STORM_FN output(Listing *src, Output *to) const;

			/**
			 * Outputs.
			 */

			virtual LabelOutput *STORM_FN labelOutput() const;
			virtual CodeOutput *STORM_FN codeOutput(Binary *owner, Array<Nat> *offsets, Nat size, Nat refs) const;

			/**
			 * Registers.
			 */

			virtual void STORM_FN removeFnRegs(RegSet *from) const;

		};

	}
}
