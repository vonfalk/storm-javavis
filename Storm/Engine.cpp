#include "stdafx.h"
#include "Engine.h"
#include "Lib/StdLib.h"

namespace storm {

	Engine::Engine(const Path &root) : rootPath(root), rootPkg(root, &rootPkg) {
		defaultOrder.scopes.push_back(&rootPkg);
		defaultOrder.scopes.push_back(package(Name(L"core"), true));

		addStdLib(*this);
	}

	Engine::~Engine() {}

	Package *Engine::package(const Name &path, bool create) {
		Named *n = rootPkg.find(path);
		if (Package *pkg = as<Package>(n))
			return pkg;

		if (create)
			return createPackage(&rootPkg, path);

		return null;
	}

	Package *Engine::createPackage(Package *pkg, const Name &path, nat pos) {
		if (path.size() == pos)
			return pkg;

		Package *next = pkg->childPackage(path[pos]);
		if (next == null) {
			next = new Package(&rootPkg);
			pkg->add(next, path[pos]);
		}

		return createPackage(next, path, pos + 1);
	}

}
