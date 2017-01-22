#include "stdafx.h"
#include "Repl.h"

namespace storm {

	LangRepl::LangRepl() {}

	LangRepl::LangRepl(Par<LangRepl> o) {}

	Bool LangRepl::eval(Par<Str> line) {
		return false;
	}

	Bool LangRepl::exit() {
		return true;
	}

}
