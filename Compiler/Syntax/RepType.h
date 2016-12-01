#pragma once
#include "Core/StrBuf.h"

namespace storm {
	namespace syntax {
		STORM_PKG(lang.bnf);

		/**
		 * Kind of repeat for an option. This should be a simple enum whenever Storm properly
		 * supports that.
		 */
		enum RepType {
			repNone,
			repZeroOne,
			repOnePlus,
			repZeroPlus,
		};

		// Output.
		StrBuf &STORM_FN operator <<(StrBuf &to, RepType r);

		// Skippable?
		Bool STORM_FN skippable(RepType rep);

		// Repeatable?
		Bool STORM_FN repeatable(RepType rep);

	}
}
