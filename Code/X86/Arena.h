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
			virtual Output *STORM_FN codeOutput(Array<Nat> *offsets, Nat size) const;

		};

		/**
		 * X86-specific registers.
		 *
		 * TODO: Expose to Storm somehow.
		 */
		extern const Register ptrD;
		extern const Register ptrSi;
		extern const Register ptrDi;
		extern const Register edx;
		extern const Register esi;
		extern const Register edi;

		// Convert to names.
		const wchar *nameX86(Register r);

		// Find unused registers to work with. Returns noReg if none. Takes 64-bit registers into account.
		Register STORM_FN unusedReg(RegSet *in);

		// Add 64-bit registers if needed.
		void STORM_FN add64(RegSet *to);

		// Get the low or high 32-bit register. If 'r' is a 32-bit register, high32 returns noReg.
		Register STORM_FN low32(Register r);
		Register STORM_FN high32(Register r);
		Operand STORM_FN low32(Operand o);
		Operand STORM_FN high32(Operand o);

		// All registers.
		RegSet *STORM_FN allRegs(EnginePtr e);

		// Registers not preserved over function calls.
		RegSet *STORM_FN fnDirtyRegs(EnginePtr e);

		/**
		 * Preserve and restore registers.
		 */

		// Preserve a single register.
		Register STORM_FN preserve(Register r, RegSet *used, Listing *dest);

		// Restore a single register.
		void STORM_FN restore(Register r, Register saved, Listing *dest);

		/**
		 * Preserve a set of registers.
		 */
		class Preserve {
			STORM_VALUE;
		public:
			STORM_CTOR Preserve(RegSet *preserve, RegSet *used, Listing *dest);

			void STORM_FN restore();
		private:
			Listing *dest;
			Array<Nat> *srcReg;
			Array<Nat> *destReg;
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
