#include "stdafx.h"
#include "Engine.h"

namespace storm {

	static const size_t defaultArena = 32 * 1024 * 1024; // 32 MB should be enough for a start at least!


	Engine::Engine(const Path &root, ThreadMode mode) : gc(defaultArena) {
		gc.test();
	}

	Engine::~Engine() {}

}
