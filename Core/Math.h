#pragma once

namespace storm {
	STORM_PKG(core);

	/**
	 * Mathematical functions for the built-in floating point types. We don't expect users to
	 * include this header, as these functions are usually already present from other headers, but
	 * Storm needs them.
	 */

	// Square root.
	Float STORM_FN sqrt(Float v);
	Double STORM_FN sqrt(Double v);

	// Power.
	Float STORM_FN pow(Float base, Float exp);
	Double STORM_FN pow(Double base, Double exp);

	// Absolute value.
	Int STORM_FN abs(Int a);
	Long STORM_FN abs(Long a);
	Float STORM_FN abs(Float a);
	Double STORM_FN abs(Double a);

	// Clamp values.
	Byte STORM_FN clamp(Byte v, Byte min, Byte max);
	Int STORM_FN clamp(Int v, Int min, Int max);
	Nat STORM_FN clamp(Nat v, Nat min, Nat max);
	Long STORM_FN clamp(Long v, Long min, Long max);
	Word STORM_FN clamp(Word v, Word min, Word max);
	Float STORM_FN clamp(Float v, Float min, Float max);
	Double STORM_FN clamp(Double v, Double min, Double max);

}

