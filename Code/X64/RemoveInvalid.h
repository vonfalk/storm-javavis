#pragma once
#include "../Transform.h"
#include "../OpTable.h"
#include "../Reg.h"
#include "../Instr.h"
#include "Core/Array.h"

namespace code {
	namespace x64 {
		STORM_PKG(core.asm.x64);

		/**
		 * Transform that removes invalid or otherwise non-supported OP-codes, replacing them with
		 * an equivalent sequence of supported OP-codes. For example:
		 * - memory-memory operands
		 *   (Note: references and object references are stored in memory, and therefore count as a memory operand!)
		 * - immediate values that require 64 bits (need to be stored in memory as well)
		 */
		class RemoveInvalid : public Transform {
			STORM_CLASS;
		public:
			STORM_CTOR RemoveInvalid();

			// Start transform.
			virtual void STORM_FN before(Listing *dest, Listing *src);

			// Transform a single instruction.
			virtual void STORM_FN during(Listing *dest, Listing *src, Nat id);

			// Finalize transform.
			virtual void STORM_FN after(Listing *dest, Listing *src);

		private:
			// Used registers for each line.
			Array<RegSet *> *used;

			// Remember all 'large' constants that need to be stored at the end of the code section.
			Array<Operand> *large;

			// Label to the start of the large constants section.
			Label lblLarge;

			// Extract any large numbers from an instruction.
			Instr *extractNumbers(Instr *i);

			// Remove any references to complex parameters.
			Instr *extractComplex(Listing *dest, Instr *i, Nat line);

			// Signature for the table of transform functions.
			typedef void (RemoveInvalid::*TransformFn)(Listing *dest, Instr *instr, Nat line);

			// Transform table.
			static const OpEntry<TransformFn> transformMap[];

			// Generic transform function for instructions using a generic two-op immReg form.
			void immRegTfm(Listing *dest, Instr *instr, Nat line);

			// Function calls.
			void fnParamTfm(Listing *dest, Instr *instr, Nat line);
			void fnParamRefTfm(Listing *dest, Instr *instr, Nat line);
			void fnCallTfm(Listing *dest, Instr *instr, Nat line);

			// Other transforms.
			void leaTfm(Listing *dest, Instr *instr, Nat line);
		};

	}
}