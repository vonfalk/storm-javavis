#pragma once

namespace code {

	/**
	 * Conditional flags for various op-codes.
	 */
		// Conditional for jumps and others.
	enum CondFlag {
		ifAlways,
		ifNever,

		ifOverflow,
		ifNoOverflow,
		ifEqual,
		ifNotEqual,

		// Unsigned comparision.
		ifBelow,
		ifBelowEqual,
		ifAboveEqual,
		ifAbove,

		// Singned comparision.
		ifLess,
		ifLessEqual,
		ifGreaterEqual,
		ifGreater,
	};

	// Get the string name.
	const wchar_t *name(CondFlag cond);

	// Inverse the flag.
	CondFlag inverse(CondFlag cond);

}
