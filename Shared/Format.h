#pragma once
#include "EnginePtr.h"
#include "Str.h"

namespace storm {
	STORM_PKG(core);

	/**
	 * Various formatting utilities.
	 */


	// Interpret a Nat or Word as a size in bytes.
	Str *STORM_ENGINE_FN toBytes(EnginePtr e, Word size);
	String toBytes(Word size);

}
