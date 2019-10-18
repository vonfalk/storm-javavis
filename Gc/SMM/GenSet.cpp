#include "stdafx.h"
#include "GenSet.h"

#if STORM_GC == STORM_GC_SMM

namespace storm {
	namespace smm {

		wostream &operator <<(wostream &to, GenSet genSet) {
			bool first = true;
			for (size_t i = 0; i < GenSet::maxGen; i++) {
				if (genSet.has(byte(i))) {
					if (!first)
						to << L" ";
					first = false;
					to << i;
				}
			}

			if (first)
				to << L"(empty)";
			return to;
		}

	}
}

#endif
