#include "stdafx.h"
#include "Package.h"

#include "BnfReader.h"

namespace storm {

	Package::Package() : pkgPath(null) {
		init();
	}

	Package::Package(const Path &path) {
		pkgPath = new Path(path);
		pkgPath->makeDir();
		init();
	}

	void Package::init() {
		syntaxLoaded = false;
		codeLoaded = false;
	}

	Package::~Package() {
		clearMap(packages);
		clearMap(syntaxTypes);
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

	Package *Package::find(const PkgPath &path, nat start) {
		if (start == path.size())
			return this;
		if (start > path.size())
			return null;

		Package *found = null;
		PkgMap::iterator i = packages.find(path[start]);
		if (i == packages.end())
			found = loadPackage(path[start]);
		else
			found = i->second;

		if (found == null)
			return null;

		return found->find(path, start + 1);
	}

	Package *Package::loadPackage(const String &name) {
		assert(packages.count(name) == 0);

		// Virtual package, can not be auto loaded.
		if (pkgPath == null)
			return null;

		Path p = *pkgPath + Path(name);
		if (!p.exists())
			return null;

		Package *pkg = new Package(*pkgPath + Path(name));
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
