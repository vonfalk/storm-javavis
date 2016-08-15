#include "stdafx.h"
#include "Package.h"
#include "Engine.h"
#include "Core/StrBuf.h"

namespace storm {

	Package::Package(Str *name) : NameSet(name) {}

	Package::Package(Url *path) : NameSet(path->name()), pkgPath(path) {}

	NameLookup *Package::parent() const {
		// We need to be able to return null.
		return parentLookup;
	}

	Url *Package::url() const {
		return pkgPath;
	}

	Bool Package::loadName(SimplePart *part) {
		// We're only loading packages this way.
		if (part->params->empty()) {
			if (Package *pkg = loadPackage(part->name)) {
				add(pkg);
				return true;
			}
		}

		// There may be other things to load!
		return false;
	}

	Bool Package::loadAll() {
		// Do not load things before the compiler has properly started. As everything is a chaos at
		// that time, loading anything external will most likely fail. So we retry later.
		if (!engine().has(bootDone))
			return false;

		// Nothing to load.
		if (!pkgPath)
			return true;

		Array<Url *> *children = pkgPath->children();

		// Load any remaining packages:
		for (nat i = 0; i < children->count(); i++) {
			Url *now = children->at(i);
			if (now->dir()) {
				Str *name = now->name();
				if (!tryFind(new (this) SimplePart(name)))
					add(loadPackage(name));
			}
		}

		// Load all code.
		loadFiles(children);

		return true;
	}

	void Package::toS(StrBuf *to) const {
		if (parent())
			*to << L"Pkg " << identifier();
		else
			*to << L"Root package";

		if (pkgPath)
			*to << L" in " << pkgPath;
		else
			*to << L"(virtual)";
		*to << L"\n";

		{
			Indent z(to);
			NameSet::toS(to);
		}
	}

	Package *Package::loadPackage(Str *name) {
		// Virtual package, can not be auto loaded.
		if (!pkgPath)
			return null;

		// TODO: Make sure this is case sensitive!
		Url *sub = pkgPath->push(name);
		if (!sub->exists())
			return null;

		return new (this) Package(sub);
	}

	void Package::loadFiles(Array<Url *> *files) {
		TODO(L"Load stuff here!");
	}

}
