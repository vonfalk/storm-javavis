#include "stdafx.h"
#include "Math.h"

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
}
