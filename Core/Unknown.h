#pragma once

namespace storm {
	STORM_PKG(core.lang.unknown);

	/**
	 * Declare the Unknown types for the preprocessor.
	 *
	 * These types correspond to members declared UNKNOWN(PTR_GC), UNKNOWN(PTR_NOGC) and
	 * UNKNOWN(INT) respectively.
	 */

	STORM_UNKNOWN_PRIMITIVE(PTR_GC, createUnknownGc);
	STORM_UNKNOWN_PRIMITIVE(PTR_NOGC, createUnknownNoGc);
	STORM_UNKNOWN_PRIMITIVE(INT, createUnknownInt);

}
