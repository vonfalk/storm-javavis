#pragma once
#include "Decl.h"
#include "Core/Io/Url.h"

namespace storm {
	namespace syntax {
		STORM_PKG(lang.bnf);

		// Parse a syntax file. Throws an exception on error.
		FileContents *STORM_FN parseSyntax(Str *content, Url *url, Str::Iter start);

	}
}
