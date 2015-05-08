#include "stdafx.h"
#include "Package.h"

#include "BnfReader.h"
#include "Type.h"
#include "Function.h"
#include "PkgReader.h"
#include "Engine.h"
#include "Exception.h"

#include "Code/FnParams.h"

namespace storm {

	Package::Package(const String &name) : NameSet(name), pkgPath(null), syntaxLoaded(false) {}

	Package::Package(Par<Url> path) : NameSet(steal(path->title())->v), pkgPath(path), syntaxLoaded(false) {}

	NameLookup *Package::parent() const {
		return parentLookup;
	}

	Url *Package::url() const {
		return pkgPath.ret();
	}

	Package::~Package() {}

	void Package::output(std::wostream &to) const {
		if (parent() == null)
			to << "Root package";
		else
			to << "Pkg " << identifier();

		if (pkgPath)
			to << " in " << *pkgPath;
		else
			to << " (virtual)";
		to << endl;

		{
			Indent i(to);
			NameSet::output(to);
		}

		if (!syntaxRules.empty()) {
			to << "Syntax:" << endl;
			Indent i(to);
			to << syntaxRules;
		}
	}

	Named *Package::loadName(const String &name, const vector<Value> &params) {
		// We are only loading packages this way at the moment.
		if (params.size() == 0) {
			if (Package *pkg = loadPackage(name)) {
				return pkg;
			}
		}

		return null;
	}

	bool Package::loadAll() {
		// Do not load things during compiler startup. Doing that will most probably fail
		// since we are in the process of setting things up.
		if (!engine().initialized())
			return false;

		if (!pkgPath)
			return true;

		Auto<ArrayP<Url>> children = pkgPath->children();

		// Load any remaining packages...
		for (nat i = 0; i < children->count(); i++) {
			Auto<Url> &now = children->at(i);
			if (now->dir()) {
				Auto<Str> name = now->name();
				if (tryFind(name->v, vector<Value>()) == null) {
					add(steal(loadPackage(name->v)));
				}
			}
		}

		// Load all code.
		loadFiles(children);

		return true;
	}

	Package *Package::loadPackage(const String &name) {
		// Virtual package, can not be auto loaded.
		if (pkgPath == null)
			return null;

		// TODO: Make sure this is case sensitive!
		Auto<Url> sub = pkgPath->push(name);
		if (!sub->exists())
			return null;

		return CREATE(Package, this, sub);
	}

	const SyntaxRules &Package::syntax() {
		if (!syntaxLoaded)
			forceLoad();
		return syntaxRules;
	}

	SyntaxRules &Package::loadSyntaxTo() {
		return syntaxRules;
	}

	void Package::loadFiles(Auto<ArrayP<Url>> children) {
		typedef hash_map<Auto<Name>, Auto<PkgFiles> > M;
		M files;
		vector<Auto<PkgReader> > toLoad;

		try {
			files = syntaxPkg(children);

			Auto<Name> myName = path();

			for (M::iterator i = files.begin(); i != files.end(); ++i) {
				if (*(i->first) != *myName)
					addReader(toLoad, i->first, i->second);
			}

			if (files.count(myName) == 1)
				addReader(toLoad, myName, files[myName]);

			// Load all syntax.
			for (nat i = 0; i < toLoad.size(); i++) {
				toLoad[i]->readSyntax();
			}

			syntaxLoaded = true;

			// Load all types.
			for (nat i = 0; i < toLoad.size(); i++) {
				toLoad[i]->readTypes();
			}

			// Set up all types.
			for (nat i = 0; i < toLoad.size(); i++) {
				toLoad[i]->resolveTypes();
			}

			// Load all functions.
			for (nat i = 0; i < toLoad.size(); i++) {
				toLoad[i]->readFunctions();
			}

		} catch (...) {
			// We did nothing...
			syntaxRules.clear();
			NameSet::clear();
			syntaxLoaded = false;
			TODO(L"We can not clear everything here, that removes potential built-in types in the same pkg!");
			throw;
		}
	}

	PkgReader *Package::createReader(Par<Name> pkg, Par<PkgFiles> files) {
		Auto<Name> rName = readerName(pkg);
		Type *readerT = as<Type>(engine().scope()->find(rName));
		if (!readerT) {
			// Ignore files that are not known.
			WARNING(L"Ignoring unknown filetype due to missing " << rName << L"(" << *files << L")");
			return null;
		}
		if (!readerT->isA(PkgReader::stormType(this)))
			throw RuntimeError(::toS(rName) + L" is not a subtype of lang.PkgReader.");

		vector<Value> paramTypes(3);
		paramTypes[0] = Value::thisPtr(readerT);
		paramTypes[1] = Value(PkgFiles::stormType(this));
		paramTypes[2] = Value(Package::stormType(this));

		Function *ctor = as<Function>(readerT->find(Type::CTOR, paramTypes));
		if (!ctor)
			throw RuntimeError(::toS(rName) + L": no constructor taking PkgFiles found!");

		code::FnParams params;
		params.add(files.borrow());
		params.add(this);
		PkgReader *r = create<PkgReader>(ctor, params);
		return r;
	}

	void Package::addReader(vector<Auto<PkgReader> > &to, Par<Name> pkg, Par<PkgFiles> files) {
		PkgReader *r = createReader(pkg, files);
		if (r)
			to.push_back(r);
	}

	/**
	 * Helpers.
	 */

	Package *rootPkg(EnginePtr e) {
		Package *root = e.v.rootPackage();
		root->addRef();
		return root;
	}

}
