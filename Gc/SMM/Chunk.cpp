#include "stdafx.h"
#include "Chunk.h"

#if STORM_GC == STORM_GC_SMM

namespace storm {
	namespace smm {

		wostream &operator <<(wostream &to, const Chunk &c) {
			return to << L"Chunk: " << c.at << L"+" << (void *)c.size;
		}

	}
}

#endif
