#pragma once

namespace storm {
	STORM_PKG(core.lang.unknown);

	/**
	 * Declare the Unknown types for the preprocessor.
	 *
	 * These types correspond to members declared UNKNOWN(PTR_GC), UNKNOWN(PTR_NOGC) and
	 * UNKNOWN(INT) respectively.
	 *
	 * Due to the typedefs below, they can also be used as generic pointer parameters to
	 * functions. However, there is no real difference between PTR_GC and PTR_NOGC there. Functions
	 * taking these types as parameters will only be callable from machine code.
	 */

	STORM_UNKNOWN_PRIMITIVE(PTR_GC, createUnknownGc);
	STORM_UNKNOWN_PRIMITIVE(PTR_NOGC, createUnknownNoGc);
	STORM_UNKNOWN_PRIMITIVE(INT, createUnknownInt);

	typedef void *PTR_GC;
	typedef void *PTR_NOGC;
	typedef int INT;
}
