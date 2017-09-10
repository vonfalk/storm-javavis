#pragma once
#include "../Transform.h"
#include "../OpTable.h"
#include "../UsedRegs.h"

namespace code {
	class Binary;

	namespace x64 {
		STORM_PKG(core.asm.x64);

		/**
		 * Transform all accesses to local variables into ebp-relative addresses. In the process,
		 * also generates function prolog and epilog as well as any construction/destruction
		 * required for the blocks in the listing.
		 *
		 * Note: Make sure not to add any extra used registers here, as this may cause the prolog
		 * and/or epilog to fail preserving some registers.
		 *
		 * Note: This should be the last transform run on a listing because of the above.
		 */
		class Layout : public Transform {
			STORM_CLASS;
		public:
			STORM_CTOR Layout(Binary *owner);

			// Start transform.
			virtual void STORM_FN before(Listing *dest, Listing *src);

			// Transform one instruction.
			virtual void STORM_FN during(Listing *dest, Listing *src, Nat id);

			// When done. Adds metadata.
			virtual void STORM_FN after(Listing *dest, Listing *src);

		private:
			// Owner. We need to stor it when using exceptions.
			Binary *owner;

			// Signature of the transform functions.
			typedef void (Layout::*TransformFn)(Listing *dest, Listing *src, Nat line);

			// Transform table.
			static const OpEntry<TransformFn> transformMap[];

			// Transform functions.
			void prologTfm(Listing *dest, Listing *src, Nat line);
			void epilogTfm(Listing *dest, Listing *src, Nat line);
			void beginBlockTfm(Listing *dest, Listing *src, Nat line);
			void endBlockTfm(Listing *dest, Listing *src, Nat line);

			// Alter a single operand. Replace any local variables with their offset.
			Operand resolve(Listing *src, const Operand &op);
		};

	}
}
