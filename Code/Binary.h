#pragma once
#include "Core/TObject.h"
#include "Arena.h"
#include "Listing.h"
#include "RefSource.h"

namespace code {
	STORM_PKG(core.asm);

	/**
	 * A Binary represents a Listing that has been translated into machine code, along with any
	 * extra information needed (such as descriptions of exception handlers or similar).
	 */
	class Binary : public Content {
		STORM_CLASS;
	public:
		// Translate a listing into machine code.
		STORM_CTOR Binary(Arena *arena, Listing *src);
	};

}
