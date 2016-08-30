#pragma once
#include "../Arena.h"
#include "../Register.h"

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

	}
}
