#include "stdafx.h"
#include "Engine.h"

namespace storm {

	Engine::Engine(const Path &root) : rootPath(root), rootPkg(root) {}

	Engine::~Engine() {}

	Package *Engine::package(const PkgPath &path) {
		return rootPkg.find(path);
	}

}
