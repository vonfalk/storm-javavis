#include "stdafx.h"
#include "ScanState.h"

#if STORM_GC == STORM_GC_SMM

namespace storm {
	namespace smm {

		ScanState::ScanState(Generation *from, Generation *to) : sourceGen(from), targetGen(to), block(null) {}

	}
}

#endif
