#pragma once
#include "../Transform.h"
#include "../OpTable.h"
#include "../UsedRegisters.h"

namespace code {
	namespace x86 {
		STORM_PKG(core.asm.x86);

		/**
		 * Transform all accesses to local variables into accesses relative to ebp. In the process,
		 * also generates function prolog and epilog, as well as any construction/destruction
		 * required for the blocks in here.
		 *
		 * Note: Make sure not to add any extra used registers here, as this may cause the prolog
		 * and/or epilog to fail preserving some registers.
		 *
		 * Note: This should be the last transform run on a listing, because of the above notice.
		 */
		class LayoutVars : public Transform {
			STORM_CLASS;
		public:
			STORM_CTOR LayoutVars(Binary *owner);

			// Start transform.
			virtual void STORM_FN before(Listing *dest, Listing *src);

			// Transform one instruction.
			virtual void STORM_FN during(Listing *dest, Listing *src, Nat id);

			// When done. Adds metadata.
			virtual void STORM_FN after(Listing *dest, Listing *src);

		private:
			// Owner. We need to store it when using exceptions.
			Binary *owner;

			// Registers saved in the prolog.
			RegSet *preserved;
			nat preservedCount;

			// Are we using an exception handler?
			bool usingEH;

			// Layout of variables.
			Array<Offset> *layout;

			// Current part.
			Part part;

			// Offset at which the part id is stored if we're using exceptions.
			Offset partId;

			// Label to where the Binary pointer is stored. Only valid if 'usingEH' is true.
			Label binaryLbl;

			// Signature for transform functions.
			typedef void (LayoutVars::*TransformFn)(Listing *dest, Listing *src, Nat line);

			// Transform table.
			static const OpEntry<TransformFn> transformMap[];

			// Transform functions.
			void prologTfm(Listing *dest, Listing *src, Nat line);
			void epilogTfm(Listing *dest, Listing *src, Nat line);
			void beginBlockTfm(Listing *dest, Listing *src, Nat line);
			void endBlockTfm(Listing *dest, Listing *src, Nat line);

			// Lookup variables to their corresponding offsets.
			Operand resolve(Listing *src, const Operand &op);

			// Initialize a block or a part.
			void initPart(Listing *dest, Part p);

			// Destroy a part.
			void destroyPart(Listing *dest, Part p, bool preserveEax);
		};

	}
}
