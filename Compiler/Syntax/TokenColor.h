#pragma once
#include "Core/EnginePtr.h"

namespace storm {
	namespace syntax {
		STORM_PKG(lang.bnf);

		/**
		 * Color of a token.
		 *
		 * TODO: Allow creation of token color descriptions by the user. These are then placed in
		 * the name tree somewhere and resolved using normal name resolution rules.
		 */
		enum TokenColor {
			tNone,
			tComment,
			tDelimiter,
			tString,
			tConstant,
			tKeyword,
			tFnName,
			tVarName,
			tTypeName,
		};

		// Conversion of token colors <=> strings.
		Str *STORM_FN name(EnginePtr e, TokenColor c);
		TokenColor STORM_FN tokenColor(Str *name);

	}
}
