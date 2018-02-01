#pragma once

namespace storm {
	STORM_PKG(core);

	/**
	 * Random number generation.
	 * TODO: Better random source, these should probably use the <random> header in the future.
	 * TODO: When we have ranges, update to support ranges.
	 * TODO: Rename to 'random'?
	 */

	// Random integer in range (including min, excluding max).
	Int STORM_FN rand(Int min, Int max);

	// Random nat in range (including min, excluding max).
	Nat STORM_FN rand(Nat min, Nat max);

	// Random float in range (including min, excluding max).
	Float STORM_FN rand(Float min, Float max);

}
