#pragma once
#include "Core/Object.h"
#include "Core/Array.h"
#include "Instr.h"

namespace code {
	STORM_PKG(core.asm);

	/**
	 * Represents a code listing along with information about blocks and variables. Can be linked
	 * into machine code.
	 */
	class Listing : public Object {
		STORM_CLASS;
	public:
		// Create an empty listing.
		STORM_CTOR Listing();


	private:
		// Instructions in here.
		Array<Instr *> *code;
	};

}
