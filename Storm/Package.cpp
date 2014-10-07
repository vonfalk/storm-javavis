#include "stdafx.h"
#include "Package.h"

#include "BnfReader.h"
#include "Type.h"

namespace storm {

	Package::Package(Scope *root) : pkgPath(null) {
		init(root);
	}

	Package::Package(const Path &path, Scope *root) {
		pkgPath = new Path(path);
		pkgPath->makeDir();
		init(root);
	}

	void Package::init(Scope *root) {
		nameFallback = root;
		syntaxLoaded = false;
		codeLoaded = false;
	}

	Package::~Package() {
		clearMap(packages);
		clearMap(syntaxTypes);
		clearMap(types);
		delete pkgPath;
	}

	void Package::output(std::wostream &to) const {
		if (pkgPath)
			to << "Pkg in " << *pkgPath;
		else
			to << "Virtual Pkg";

		to << endl;
		to << "Loaded packages:" << endl;
		for (PkgMap::const_iterator i = packages.begin(); i != packages.end(); ++i) {
			to << "->" << i->first << endl;
		}
	}

	Named *Package::findHere(const Name &name) {
		return findName(name, 0);
	}

	Named *Package::findName(const Name &name, nat start) {
		if (start == name.size())
			return this;
		if (start > name.size())
			return null;

		Package *found = null;
		PkgMap::iterator i = packages.find(name[start]);
		if (i == packages.end())
			found = loadPackage(name[start]);
		else
			found = i->second;

		if (found == null)
			return findTypeOrFn(name, start);

		return found->findName(name, start + 1);
	}

	Named *Package::findTypeOrFn(const Name &name, nat start) {
		// Nested types are not yet supported.
		if (start != name.size() - 1)
			return null;

		TypeMap::iterator i = types.find(name[start]);
		if (i == types.end())
			return null;
		else
			return i->second;
	}

	Package *Package::loadPackage(const String &name) {
		assert(packages.count(name) == 0);

		// Virtual package, can not be auto loaded.
		if (pkgPath == null)
			return null;

		Path p = *pkgPath + Path(name);
		if (!p.exists())
			return null;

		Package *pkg = new Package(*pkgPath + Path(name), nameFallback);
		packages[name] = pkg;
		return pkg;
	}

	hash_map<String, SyntaxType*> Package::syntax() {
		if (!syntaxLoaded)
			loadSyntax();

		return syntaxTypes;
	}

	void Package::loadSyntax() {
		if (!pkgPath)
			return;

		try {
			vector<Path> files = pkgPath->children();
			for (nat i = 0; i < files.size(); i++) {
				const Path &f = files[i];
				if (!f.isDir() && isBnfFile(f)) {
					parseBnf(syntaxTypes, f);
				}
			}
		} catch (...) {
			clearMap(syntaxTypes);
			throw;
		}

		syntaxLoaded = true;
	}
}
