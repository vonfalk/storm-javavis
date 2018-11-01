#include "stdafx.h"
#include "StormMath.h"

namespace storm {

	Float sqrt(Float v) {
		return ::sqrt(v);
	}

	Double sqrt(Double v) {
		return ::sqrt(v);
	}

	Float pow(Float base, Float exp) {
		return ::pow(base, exp);
	}

	Double pow(Double base, Double exp) {
		return ::pow(base, exp);
	}

	Int abs(Int a) {
		return (a >= 0) ? a : -a;
	}

	Long abs(Long a) {
		return (a >= 0) ? a : -a;
	}

	Float abs(Float a) {
		return (a >= 0) ? a : -a;
	}

	Double abs(Double a) {
		return (a >= 0) ? a : -a;
	}

	Byte clamp(Byte v, Byte min, Byte max) {
		return ::min(::max(v, min), max);
	}

	Int clamp(Int v, Int min, Int max) {
		return ::min(::max(v, min), max);
	}

	Nat clamp(Nat v, Nat min, Nat max) {
		return ::min(::max(v, min), max);
	}

	Long clamp(Long v, Long min, Long max) {
		return ::min(::max(v, min), max);
	}

	Word clamp(Word v, Word min, Word max) {
		return ::min(::max(v, min), max);
	}

	Float clamp(Float v, Float min, Float max) {
		return ::min(::max(v, min), max);
	}

	Double clamp(Double v, Double min, Double max) {
		return ::min(::max(v, min), max);
	}

}
