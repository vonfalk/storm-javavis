#pragma once
#include "Core/Array.h"
#include "Core/Io/Url.h"
#include "Compiler/Package.h"

namespace storm {
	namespace syntax {
		STORM_PKG(lang.bnf);

		// Entry point for the syntax language.
		PkgReader *STORM_FN reader(Array<Url *> *files, Package *into);

	}
}
