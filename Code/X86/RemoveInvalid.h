#pragma once
#include "../Transform.h"
#include "../OpTable.h"
#include "../UsedRegisters.h"

namespace code {
	namespace x86 {
		STORM_PKG(core.asm.x86);

		/**
		 * Transform which replaces invalid instructions with a sequence of valid ones.
		 */
		class RemoveInvalid : public Transform {
			STORM_CLASS;
		public:
			STORM_CTOR RemoveInvalid();

			// Start transform.
			virtual void STORM_FN before(Listing *dest, Listing *src);

			// Transform one instruction.
			virtual void STORM_FN during(Listing *dest, Listing *src, Nat id);

		private:
			// Remember used registers.
			Array<RegSet *> *used;

			// Find an unused register at 'line'.
			Register unusedReg(Nat line);

			// Struct describing parameters for a future fnCall instruction.
			class Param {
				STORM_VALUE;
			public:
				Param(Operand src, Operand copyFn, Bool ref);

				// Source operand.
				Operand src;

				// Function used for copying values (if any).
				Operand copyFn;

				// Reference parameter?
				Bool ref;
			};

			// Function params seen so far.
			Array<Param> *params;

			// Signature for the table of transform functions.
			typedef void (RemoveInvalid::*TransformFn)(Listing *dest, Instr *instr, Nat line);

			// Transform table.
			static const OpEntry<TransformFn> transformMap[];

			// Transform functions.
			void immRegTfm(Listing *dest, Instr *instr, Nat line);
			void leaTfm(Listing *dest, Instr *instr, Nat line);
			void mulTfm(Listing *dest, Instr *instr, Nat line);
			void idivTfm(Listing *dest, Instr *instr, Nat line);
			void udivTfm(Listing *dest, Instr *instr, Nat line);
			void imodTfm(Listing *dest, Instr *instr, Nat line);
			void umodTfm(Listing *dest, Instr *instr, Nat line);
			void setCondTfm(Listing *dest, Instr *instr, Nat line);
			void shlTfm(Listing *dest, Instr *instr, Nat line);
			void shrTfm(Listing *dest, Instr *instr, Nat line);
			void sarTfm(Listing *dest, Instr *instr, Nat line);
			void icastTfm(Listing *dest, Instr *instr, Nat line);
			void ucastTfm(Listing *dest, Instr *instr, Nat line);
			void callFloatTfm(Listing *dest, Instr *instr, Nat line);
			void retFloatTfm(Listing *dest, Instr *instr, Nat line);

			// Function calls.
			void fnParamTfm(Listing *dest, Instr *instr, Nat line);
			void fnParamRefTfm(Listing *dest, Instr *instr, Nat line);
			void fnCallTfm(Listing *dest, Instr *instr, Nat line);
			void fnCallFloatTfm(Listing *dest, Instr *instr, Nat line);
		};

	}
}
