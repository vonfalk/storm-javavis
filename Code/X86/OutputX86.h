#pragma once
#include "../Output.h"

namespace code {
	namespace x86 {
		STORM_PKG(core.asm.x86);

		class CodeOutput : public Output {
			STORM_CLASS;
		public:
			STORM_CTOR CodeOutput(Array<Nat> *lbls, Nat size);

			// TODO!
		};

	}
}
