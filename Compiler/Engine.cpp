#include "stdafx.h"
#include "Engine.h"
#include "Init.h"

namespace storm {

	static const size_t defaultArena = 32 * 1024 * 1024; // 32 MB should be enough for a start at least!


	Engine::Engine(const Path &root, ThreadMode mode) : gc(defaultArena), cppTypes(gc) {
		// Initialize the type system. This loads all types defined in the compiler.
		initTypes(*this, cppTypes);
	}

	Engine::~Engine() {
		// We need to remove the root this array implies before the Gc is destroyed.
		cppTypes.clear();
	}

}
