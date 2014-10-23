#include "stdafx.h"
#include "Engine.h"
#include "Std.h"
#include "Exception.h"

namespace storm {

	Engine::Engine(const Path &root) : rootPath(root), rootPkg(root, &defaultPkgs) {
		defaultPkgs.pkgs.push_back(package(Name(L"core"), true));
		addStdLib(*this);

		setType(tStr, L"core.Str");
		setType(tType, L"core.Type");
	}

	Engine::~Engine() {}

	void Engine::setType(Type *&t, const String &name) {
		Named *n = scope()->find(Name(name));
		if (Type *tt = as<Type>(n)) {
			t = tt;
		} else {
			throw InternalError(L"Failed to find " + name);
		}
	}

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

	Named *Engine::DefaultPkgs::findHere(const Name &name) {
		for (nat i = 0; i < pkgs.size(); i++) {
			Named *n = pkgs[i]->findHere(name);
			if (n)
				return n;
		}
		return null;
	}

	Type *Engine::strType() {
		return tStr;
	}

	Type *Engine::typeType() {
		return tType;
	}

}
