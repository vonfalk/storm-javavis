#include "stdafx.h"
#include "Thread.h"
#include "NamedThread.h"
#include "Engine.h"
#include "Scope.h"
#include "Exception.h"

namespace storm {

	DEFINE_STORM_THREAD(Compiler);

	NamedThread *STORM_ENGINE_FN compilerThread(EnginePtr e) {
		Auto<SimpleName> name = parseSimpleName(e.v, L"core.Compiler");
		Auto<Named> found = e.v.scope()->find(name);

		if (NamedThread *t = as<NamedThread>(found.borrow())) {
			return t;
		}

		throw RuntimeError(L"FATAL: core.Compiler was not found!");
	}

}
