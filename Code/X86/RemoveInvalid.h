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
			// Remember unused registers.
			Array<RegSet *> *used;

			// Find an unused register at 'line'.
			Register unusedReg(Nat line);

			// Signature for the table of transform functions.
			typedef void (RemoveInvalid::*TransformFn)(Listing *dest, Listing *src, Nat line);

			// Transform table.
			static const OpEntry<TransformFn> transformMap[];

			// Transform functions.
			void immRegTfm(Listing *dest, Listing *src, Nat line);
			void leaTfm(Listing *dest, Listing *src, Nat line);
			void mulTfm(Listing *dest, Listing *src, Nat line);
			void idivTfm(Listing *dest, Listing *src, Nat line);
			void udivTfm(Listing *dest, Listing *src, Nat line);
			void imodTfm(Listing *dest, Listing *src, Nat line);
			void umodTfm(Listing *dest, Listing *src, Nat line);
			void setCondTfm(Listing *dest, Listing *src, Nat line);
			void shlTfm(Listing *dest, Listing *src, Nat line);
			void shrTfm(Listing *dest, Listing *src, Nat line);
			void sarTfm(Listing *dest, Listing *src, Nat line);
			void icastTfm(Listing *dest, Listing *src, Nat line);
			void ucastTfm(Listing *dest, Listing *src, Nat line);
			void callFloatTfm(Listing *dest, Listing *src, Nat line);
			void retFloatTfm(Listing *dest, Listing *src, Nat line);
			void fnCallFloatTfm(Listing *dest, Listing *src, Nat line);
		};

	}
}
