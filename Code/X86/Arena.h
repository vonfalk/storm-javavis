#pragma once
#include "../Arena.h"
#include "../Register.h"
#include "../Operand.h"

namespace code {
	namespace x86 {
		STORM_PKG(core.asm.x86);

		/**
		 * Arena for X86 (TODO: factor windows/unix versions of this one!)
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
			virtual CodeOutput *STORM_FN codeOutput(Array<Nat> *offsets, Nat size, Nat refs) const;

			/**
			 * Registers.
			 */

			virtual void STORM_FN removeFnRegs(RegSet *from) const;

		};


		/**
		 * Variable layouts. Extends code::layout by also providing offsets for parameters, and
		 * aligns the naive offsets returned from there to what actually happens on an X86 using
		 * cdecl calling convention.
		 *
		 * 'savedRegs' - # of saved registers in this case.
		 * 'usingEh' - using exception handler (on Windows)?
		 */
		Array<Offset> *STORM_FN layout(Listing *src, Nat savedRegs, Bool usingEh);


	}
}
