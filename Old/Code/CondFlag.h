#pragma once

namespace code {

	/**
	 * Conditional flags for various op-codes.
	 */
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

		// Float comparision.
		ifFBelow,
		ifFBelowEqual,
		ifFAboveEqual,
		ifFAbove,
	};

	// Get the string name.
	const wchar_t *name(CondFlag cond);

	// Inverse the flag.
	CondFlag inverse(CondFlag cond);

}
