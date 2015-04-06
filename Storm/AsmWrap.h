#pragma once
#include "Code/Listing.h"
#include "Code/Instruction.h"
#include "Lib/Object.h"

namespace storm {

	namespace wrap {

		// Wrapper for the code listing.
		STORM_PKG(core.asm);

		// Instruction.
		class Instruction {
			STORM_VALUE;
		public:
			Instruction(const code::Instruction &v);

			// Value.
			code::Instruction v;
		};

		// Create instructions.
		Instruction STORM_FN prolog();
		Instruction STORM_FN epilog();
		Instruction STORM_FN ret();

		// Listing
		class Listing : public Object {
			STORM_CLASS;
		public:
			// Crate.
			STORM_CTOR Listing();
			STORM_CTOR Listing(Par<Listing> o);

			// Data.
			code::Listing v;

			// Append instructions.
			Listing *STORM_FN operator <<(const Instruction &v);
		};

	}
}
