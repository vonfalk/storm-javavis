#include "stdafx.h"
#include "Engine.h"
#include "Lib/StdLib.h"

namespace storm {

	Engine::Engine(const Path &root) : rootPath(root), rootPkg(root, null) {
		addStdLib(*this);
	}

	Engine::~Engine() {}

	Package *Engine::package(const Name &path) {
		Named *n = rootPkg.find(path);
		if (Package *pkg = as<Package>(n))
			return pkg;
		return null;
	}

}
