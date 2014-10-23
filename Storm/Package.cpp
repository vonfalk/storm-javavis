#include "stdafx.h"
#include "Package.h"

#include "BnfReader.h"
#include "Type.h"
#include "Function.h"

namespace storm {

	Package::Package(Scope *root) : Named(L"<root>"), pkgPath(null) {
		init(root);
	}

	Package::Package(const Path &path, Scope *root) : Named(path.title()) {
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
		clearMap(syntaxRules);
		clearMap(types);
		clearMap(members);
		delete pkgPath;
	}

	void Package::output(std::wostream &to) const {
		if (pkgPath)
			to << "Pkg " << name << " in " << *pkgPath;
		else
			to << "Virtual Pkg " << name;
		to << endl;

		to << "Loaded packages:" << endl;
		{
			Indent i(to);
			for (PkgMap::const_iterator i = packages.begin(); i != packages.end(); ++i) {
				to << *i->second << endl;
			}
		}

		to << "Types:" << endl;
		{
			Indent i(to);
			for (TypeMap::const_iterator i = types.begin(); i != types.end(); ++i) {
				to << *i->second << endl;
			}
		}

		to << "Members:" << endl;
		{
			Indent i(to);
			for (MemberMap::const_iterator i = members.begin(); i != members.end(); ++i) {
				to << *i->second << endl;
			}
		}

		to << "Syntax:" << endl;
		{
			Indent i(to);
			for (SyntaxMap::const_iterator i = syntaxRules.begin(); i != syntaxRules.end(); ++i) {
				to << *i->second << endl;
			}
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

		Package *found = childPackage(name[start]);
		if (found == null)
			return findTypeOrFn(name, start);

		return found->findName(name, start + 1);
	}

	Named *Package::findTypeOrFn(const Name &name, nat start) {
		// Nested types are not yet supported.
		if (start != name.size() - 1)
			return null;

		TypeMap::iterator i = types.find(name[start]);
		if (i != types.end())
			return i->second;

		MemberMap::iterator j = members.find(name[start]);
		if (j != members.end())
			return j->second;

		return null;
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

	hash_map<String, SyntaxRule*> Package::syntax() {
		if (!syntaxLoaded)
			loadSyntax();

		return syntaxRules;
	}

	Package *Package::childPackage(const String &name) {
		PkgMap::iterator i = packages.find(name);
		if (i == packages.end())
			return loadPackage(name);
		else
			return i->second;
	}

	void Package::add(Package *pkg, const String &name) {
		assert(packages.count(name) == 0);
		packages.insert(make_pair(name, pkg));
	}

	void Package::add(Type *type) {
		assert(types.count(type->name) == 0);
		types.insert(make_pair(type->name, type));
		type->setParentScope(this);
	}

	void Package::add(NameOverload *fn) {
		Overload *o = null;
		MemberMap::iterator i = members.find(fn->name);
		if (i == members.end()) {
			o = new Overload(fn->name);
			members.insert(make_pair(fn->name, o));
		} else {
			o = i->second;
		}

		o->add(fn);
	}

	void Package::loadSyntax() {
		if (!pkgPath)
			return;

		try {
			vector<Path> files = pkgPath->children();
			for (nat i = 0; i < files.size(); i++) {
				const Path &f = files[i];
				if (!f.isDir() && isBnfFile(f)) {
					parseBnf(syntaxRules, f, this);
				}
			}
		} catch (...) {
			clearMap(syntaxRules);
			throw;
		}

		syntaxLoaded = true;
	}
}
