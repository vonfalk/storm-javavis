#pragma once
#include "Core/Object.h"
#include "Core/Array.h"
#include "Register.h"
#include "Listing.h"

namespace code {
	STORM_PKG(core.asm);

	/**
	 * Computes and tracks the register usage for instructions in a listing. Each entry tracks the
	 * registers that need to be preserved before each instruction. Ignores ptrStack and ptrFrame.
	 */

	class UsedRegisters {
		STORM_VALUE;
	public:
		STORM_CTOR UsedRegisters(Array<RegSet *> *used, RegSet *all);

		Array<RegSet *> *used;
		RegSet *all;
	};

	// Computes the used registers which needs to be preserved before each instruction in a
	// listing. Ignores ptrStack and ptrFrame.
	UsedRegisters STORM_FN usedRegisters(const Listing *src);

	// Computes all used registers, without bothering about per-line representations.
	RegSet *STORM_FN allUsedRegisters(const Listing *src);
}
