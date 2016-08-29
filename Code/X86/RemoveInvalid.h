#pragma once
#include "../Transform.h"
#include "../OpTable.h"

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

			// Signature for the table of transform functions.
			typedef void (RemoveInvalid::*TransformFn)(Listing *dest, Listing *src, Nat id);

			// Transform table.
			static const OpEntry<TransformFn> transformMap[];

			// Transform functions.
			void immRegTfm(Listing *dest, Listing *src, Nat id);
			void leaTfm(Listing *dest, Listing *src, Nat id);
			void mulTfm(Listing *dest, Listing *src, Nat id);
			void idivTfm(Listing *dest, Listing *src, Nat id);
			void udivTfm(Listing *dest, Listing *src, Nat id);
			void imodTfm(Listing *dest, Listing *src, Nat id);
			void umodTfm(Listing *dest, Listing *src, Nat id);
			void setCondTfm(Listing *dest, Listing *src, Nat id);
			void shlTfm(Listing *dest, Listing *src, Nat id);
			void shrTfm(Listing *dest, Listing *src, Nat id);
			void sarTfm(Listing *dest, Listing *src, Nat id);
			void icastTfm(Listing *dest, Listing *src, Nat id);
			void ucastTfm(Listing *dest, Listing *src, Nat id);
			void callFloatTfm(Listing *dest, Listing *src, Nat id);
			void retFloatTfm(Listing *dest, Listing *src, Nat id);
			void fnCallFloatTfm(Listing *dest, Listing *src, Nat id);
		};

	}
}
