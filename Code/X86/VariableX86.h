#pragma once

#ifdef X86
#include "Listing.h"

namespace code {
	namespace machineX86 {

		// Describes variable offsets.
		class Offsets {
		public:
			void init(nat preservedRegisters, const Frame &frame);

			// Get the offset from ebp for the variable or parameter 'v'.
			int offset(Variable v) const;

			// Get a value representing the resolved variable 'v'.
			Value variable(Variable v, int offset) const;

			// Get the largest required stack size.
			inline nat maxSize() const { return maxSz; }

			// Get a pointer to the hidden variable containing the current active block.
			Value blockId() const;

			// Get a pointer to the hidden variable containing the owning block object.
			Value blockPtr() const;

		private:
			// Offset for all variables and parameters (pre-computed).
			vector<int> off;

			// Maxium size required by local variables.
			nat maxSz;

			// Update the offset for 'off'. Returns the updated offset for 'var'.
			int updateVarOffset(const Variable &var, const Frame &frame, nat savedReg);
			int updateParamOffset(const Variable &var, const Frame &frame);
		};

		// Meta format.
		struct Meta {
			// Information about a single variable.
			struct Var {
				// Offset of the variable.
				int offset;

				// The function to be called (if any) to free the variable.
				void *freeFn;
			};

			// Number of elements in "info".
			nat infoCount;

			// Information about variables.
			Var info[1];

			// Write metadata.
			static void write(Listing &to, const Frame &frame, const Offsets &offsets);
		};

	}

	namespace machine {
		typedef machineX86::Meta FnMeta;
	}
}

#endif